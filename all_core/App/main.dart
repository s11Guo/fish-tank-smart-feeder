import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:flutter/material.dart';

void main() => runApp(const FishApp());

class FishApp extends StatelessWidget {
  const FishApp({super.key});
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Fish Tank',
      theme: ThemeData(
        colorSchemeSeed: Colors.teal,
        brightness: Brightness.dark,
        useMaterial3: true,
      ),
      home: const HomePage(),
    );
  }
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});
  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  static const _host = '192.168.4.1';
  static const _port = 9000;
  Socket? _client;
  bool _connecting = false;
  String _status = 'Not connected';
  Timer? _reconTimer;
  int _lastSend = 0;

  // --- Sensor data ---
  double _temp   = 0;
  double _tds    = 0;
  int    _led    = 0;
  int    _lmode  = 0;     // 0=Auto, 1=Manual
  bool   _heater = false;
  int    _pump   = 0;     // 0=off, 1=P1out, 2=P2into
  bool   _run    = false;
  String _page   = '--';

  // --- Cursor / action ---
  int _cursor = 255;
  int _sub    = 0;
  int _action = 0;
  int _flashTrigger = 0;
  int _nullFlash = 0;
  Timer? _autoNullTimer;

  String _buffer = '';
  String _rawLog = '';

  bool _synced = false;

  Future<void> _connect() async {
    if (_client != null || _connecting) return;
    _reconTimer?.cancel();
    setState(() { _connecting = true; _synced = false; _status = 'Connecting...'; });
    try {
      final s = await Socket.connect(_host, _port,
        timeout: const Duration(seconds: 5));
      s.setOption(SocketOption.tcpNoDelay, true);
      _client = s;
      _buffer = '';
      setState(() { _connecting = false; _status = 'Connected $_host:$_port'; });
      _listen(s);
      // Force immediate JSON push to sync page state
      Timer(const Duration(milliseconds: 200), () => _send('r'));
    } catch (e) {
      setState(() { _connecting = false; _status = 'Connect failed'; });
      _scheduleReconnect();
    }
  }

  void _listen(Socket s) {
    s.listen(
      (data) {
        _buffer += utf8.decode(data);
        setState(() => _rawLog = 'RAW(${data.length}B): ${utf8.decode(data)}');
        while (_buffer.contains('\n')) {
          final idx = _buffer.indexOf('\n');
          final line = _buffer.substring(0, idx).trim();
          _buffer = _buffer.substring(idx + 1);
          if (line.isEmpty) continue;
          try {
            final json = jsonDecode(line) as Map<String, dynamic>;
            final newAction = (json['action'] as num?)?.toInt() ?? 0;
            if (newAction == 1 && _action == 0) {
              _flashTrigger++;
            }
            setState(() {
              _temp   = (json['temp']   as num?)?.toDouble() ?? _temp;
              _tds    = (json['tds']    as num?)?.toDouble() ?? _tds;
              _led    = (json['led']    as num?)?.toInt()    ?? _led;
              _lmode  = (json['lmode']  as num?)?.toInt()    ?? _lmode;
              _heater = (json['heater'] as num?)?.toInt() == 1;
              _pump   = (json['pump']   as num?)?.toInt()    ?? _pump;
              _run    = (json['run']    as num?)?.toInt() == 1;
              _page   = json['page'] as String? ?? _page;
              _cursor = (json['cursor'] as num?)?.toInt() ?? _cursor;
              _sub    = (json['sub']    as num?)?.toInt() ?? _sub;
              _action = newAction;
              _synced = true;
            });
          } catch (_) {}
        }
      },
      onDone: () {
        _client = null;
        setState(() {
          _status = 'Disconnected, retry 3s...';
          _page = '--'; _cursor = 255; _sub = 0;
        });
        _scheduleReconnect();
      },
      onError: (_) {
        _client?.destroy();
        _client = null;
        setState(() { _page = '--'; _cursor = 255; _sub = 0; });
        _scheduleReconnect();
      },
    );
  }

  void _scheduleReconnect() {
    _reconTimer?.cancel();
    _reconTimer = Timer(const Duration(seconds: 3), _connect);
  }

  void _disconnect() {
    _reconTimer?.cancel();
    _client?.destroy();
    _client = null;
    setState(() => _status = 'Disconnected');
  }

  void _send(String cmd) {
    final now = DateTime.now().millisecondsSinceEpoch;
    if (now - _lastSend < 150) return;
    _lastSend = now;
    try { _client?.write(cmd + cmd); _client?.flush(); } catch (_) {}

    if (cmd != '0') {
      _autoNullTimer?.cancel();
      _autoNullTimer = Timer(const Duration(milliseconds: 60), () {
        try { _client?.write('00'); _client?.flush(); } catch (_) {}
        if (mounted) setState(() => _nullFlash = 1);
        Timer(const Duration(milliseconds: 250), () {
          if (mounted) setState(() => _nullFlash = 0);
        });
      });
    }
  }

  @override
  void dispose() {
    _reconTimer?.cancel();
    _autoNullTimer?.cancel();
    _client?.destroy();
    super.dispose();
  }

  bool _isSelected(int index) => _cursor != 255 && _cursor == index;

  // --- Derived values ---
  String get _lightStatus {
    if (_lmode == 1) return 'Custom';
    if (_led == 0)   return 'Bright';
    if (_led < 100)  return 'OK';
    return 'Low';
  }

  String get _pumpCond {
    if (_tds < 300)  return 'Good';
    if (_tds <= 600) return 'Fair';
    if (_tds <= 1000) return 'Dirty';
    return 'Poor';
  }

  String get _pumpLabel {
    switch (_pump) {
      case 1: return 'P1out';
      case 2: return 'P2into';
      default: return 'OFF';
    }
  }

  // ============ Build ============
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Fish Tank'), centerTitle: true),
      body: SafeArea(
        child: Column(
          children: [
            _connBar(),
            const Divider(height: 1),
            Expanded(child: _oledMirror()),
            if (_rawLog.isNotEmpty)
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 12),
                child: Text(_rawLog, style: const TextStyle(fontSize: 10, color: Colors.grey), maxLines: 2),
              ),
            _dpad(),
            const SizedBox(height: 8),
          ],
        ),
      ),
    );
  }

  Widget _connBar() {
    final connected = _client != null;
    return Padding(
      padding: const EdgeInsets.all(12),
      child: Row(
        children: [
          const Expanded(child: Text('$_host:$_port', style: TextStyle(fontSize: 13, color: Colors.cyan))),
          if (connected)
            IconButton(
              icon: const Icon(Icons.refresh, color: Colors.cyan, size: 22),
              tooltip: 'Refresh',
              onPressed: () => _send('r'),
            ),
          FilledButton.icon(
            onPressed: connected ? _disconnect : _connect,
            icon: Icon(connected ? Icons.stop : Icons.play_arrow),
            label: Text(connected ? 'Disconnect' : 'Connect'),
          ),
          const SizedBox(width: 8),
          Expanded(child: Text(_status,
            style: TextStyle(color: connected ? Colors.green : (_connecting ? Colors.orange : Colors.grey), fontSize: 12))),
        ],
      ),
    );
  }

  // ============ OLED Mirror ============
  Widget _oledMirror() {
    return Container(
      margin: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: const Color(0xFF0A0A12),
        border: Border.all(color: Colors.cyan.withOpacity(0.3), width: 1),
        borderRadius: BorderRadius.circular(4),
      ),
      child: Column(
        children: [
          // Top title bar
          Container(
            width: double.infinity,
            padding: const EdgeInsets.symmetric(vertical: 10, horizontal: 12),
            decoration: BoxDecoration(
              border: Border(bottom: BorderSide(color: Colors.cyan.withOpacity(0.2))),
            ),
            child: Text(_page, textAlign: TextAlign.center,
              style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Colors.cyan, letterSpacing: 3)),
          ),
          // Content
          Expanded(child: _pageContent()),
          // Bottom sensor bar
          Container(
            width: double.infinity,
            padding: const EdgeInsets.symmetric(vertical: 8, horizontal: 12),
            decoration: BoxDecoration(
              border: Border(top: BorderSide(color: Colors.cyan.withOpacity(0.2))),
            ),
            child: Row(mainAxisAlignment: MainAxisAlignment.spaceBetween, children: [
              _sensorChip(Icons.thermostat, '${_temp.toStringAsFixed(1)}C', _heater ? Colors.red : Colors.white70),
              _sensorChip(Icons.water_drop, '${_tds.toStringAsFixed(0)}ppm', Colors.white70),
              _sensorChip(Icons.light_mode, 'LED $_led%', Colors.yellow),
            ]),
          ),
        ],
      ),
    );
  }

  Widget _sensorChip(IconData icon, String text, Color color) {
    return Row(mainAxisSize: MainAxisSize.min, children: [
      Icon(icon, size: 14, color: color.withOpacity(0.7)),
      const SizedBox(width: 4),
      Text(text, style: TextStyle(fontSize: 13, color: color)),
    ]);
  }

  // ============ Page Content ============
  Widget _pageContent() {
    if (!_synced) {
      return const Center(child: Column(mainAxisSize: MainAxisSize.min, children: [
        SizedBox(width: 24, height: 24, child: CircularProgressIndicator(strokeWidth: 2, color: Colors.cyan)),
        SizedBox(height: 12),
        Text('Syncing...', style: TextStyle(fontSize: 14, color: Colors.white38)),
      ]));
    }
    switch (_page) {
      case 'Welcome':
        return Center(child: Column(mainAxisSize: MainAxisSize.min, children: [
          const Icon(Icons.waves, size: 56, color: Colors.cyan),
          const SizedBox(height: 12),
          const Text('Fish Tank', style: TextStyle(fontSize: 22, color: Colors.white60, letterSpacing: 2)),
          const SizedBox(height: 6),
          const Text('Press Enter', style: TextStyle(fontSize: 14, color: Colors.white30)),
        ]));

      case 'Menu':
        return _menuList([
          _MenuItem('Welcome', Icons.waves),
          _MenuItem('Feed', Icons.lunch_dining),
          _MenuItem('Light', Icons.lightbulb),
          _MenuItem('Temp', Icons.thermostat),
          _MenuItem('Pump', Icons.water),
        ]);

      case 'FeedSub':
        return _menuList([
          _MenuItem('Train Mode', Icons.fitness_center),
          _MenuItem('Manual Feed', Icons.touch_app),
          _MenuItem('Auto Feed', Icons.schedule),
        ]);

      case 'FeedTrain':
        return _menuList([
          _MenuItem('Feed Setting', Icons.settings),
          _MenuItem('Start', Icons.play_arrow),
          _MenuItem('End', Icons.stop),
        ], title: 'TRAIN MODE');

      case 'ManualFeed':
        return _menuList([
          _MenuItem('Feed Time', Icons.timer),
          _MenuItem('Start', Icons.play_arrow),
          _MenuItem('End', Icons.stop),
        ], title: 'MANUAL FEED');

      case 'TrainSet':
        return _menuList([
          _MenuItem('BO Time', Icons.arrow_back),
          _MenuItem('AO Time', Icons.arrow_forward),
          _MenuItem('Interval', Icons.timelapse),
          _MenuItem('Continue', Icons.arrow_forward_ios),
        ], title: 'TRAIN SETTING');

      case 'Auto':
        return Column(children: [
          const SizedBox(height: 12),
          const Text('AUTO FEED', style: TextStyle(fontSize: 18, color: Colors.cyan, letterSpacing: 2)),
          const SizedBox(height: 16),
          Expanded(child: _menuList([
            _MenuItem('Interval', Icons.timelapse),
            _MenuItem('AO Time', Icons.timer),
            _MenuItem('Start', Icons.play_arrow),
            _MenuItem('End', Icons.stop),
          ])),
          _runningBadge('Auto', _run),
        ]);

      // ── LIGHT (matches OLED) ──
      case 'Light':
        return _lightPage();

      // ── TEMP (matches OLED) ──
      case 'Temp':
        return _tempPage();

      // ── PUMP (matches OLED) ──
      case 'Pump':
        return _pumpPage();

      // --- Feed (running) ---
      case 'Feed':
        return Center(child: Column(mainAxisSize: MainAxisSize.min, children: [
          const Text('FEED CONTROL', style: TextStyle(fontSize: 18, color: Colors.cyan, letterSpacing: 2)),
          const SizedBox(height: 16),
          const Icon(Icons.lunch_dining, size: 48, color: Colors.orange),
          const SizedBox(height: 12),
          const Text('Running...', style: TextStyle(fontSize: 20, color: Colors.white70)),
        ]));

      default:
        return Center(child: Column(mainAxisSize: MainAxisSize.min, children: [
          Text(_page, style: const TextStyle(fontSize: 18, color: Colors.cyan, letterSpacing: 2)),
          const SizedBox(height: 16),
          Text('${_temp.toStringAsFixed(1)}C', style: const TextStyle(fontSize: 32, color: Colors.white70)),
          Text('${_tds.toStringAsFixed(0)}ppm', style: const TextStyle(fontSize: 16, color: Colors.white54)),
        ]));
    }
  }

  // ============ LIGHT Page ============
  Widget _lightPage() {
    final isAuto = _lmode == 0;
    final modeStr = isAuto ? 'Auto' : 'Manual';
    final status = _lightStatus;

    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 20),
      child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
        // Title row
        Row(mainAxisAlignment: MainAxisAlignment.center, children: [
          const Text('LIGHT  ', style: TextStyle(fontSize: 18, color: Colors.cyan, letterSpacing: 2)),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 3),
            decoration: BoxDecoration(
              color: isAuto ? Colors.green.withOpacity(0.15) : Colors.orange.withOpacity(0.15),
              borderRadius: BorderRadius.circular(4),
              border: Border.all(color: isAuto ? Colors.green.withOpacity(0.4) : Colors.orange.withOpacity(0.4)),
            ),
            child: Text(modeStr, style: TextStyle(fontSize: 14, color: isAuto ? Colors.green : Colors.orange, fontWeight: FontWeight.bold)),
          ),
        ]),
        const SizedBox(height: 24),

        // LED row
        _oledRow(0, 'LED', '$_led%', Icons.lightbulb, Colors.yellow),
        const SizedBox(height: 10),

        // Status row
        _oledRow(1, 'Status', status, Icons.info_outline,
          status == 'Bright' ? Colors.green :
          status == 'OK' ? Colors.cyan :
          status == 'Low' ? Colors.red : Colors.grey),
        const SizedBox(height: 10),

        // Mode row
        _oledRow(2, 'Mode', modeStr, Icons.settings,
          isAuto ? Colors.green : Colors.orange),
      ]),
    );
  }

  // ============ TEMP Page ============
  Widget _tempPage() {
    String tempStatus;
    Color statusColor;
    if (_temp < 18.0) {
      tempStatus = 'Low';  statusColor = Colors.blue;
    } else if (_temp > 35.0) {
      tempStatus = 'High'; statusColor = Colors.red;
    } else {
      tempStatus = 'OK';   statusColor = Colors.green;
    }

    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 20),
      child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
        const Text('HEATER', style: TextStyle(fontSize: 18, color: Colors.cyan, letterSpacing: 2)),
        const SizedBox(height: 20),

        _oledRow(-1, 'Target Low', '24.0', Icons.arrow_downward, Colors.blue),
        const SizedBox(height: 6),
        _oledRow(-1, 'Target High', '30.0', Icons.arrow_upward, Colors.red),
        const SizedBox(height: 16),

        Container(
          padding: const EdgeInsets.all(12),
          decoration: BoxDecoration(
            border: Border.all(color: Colors.white12),
            borderRadius: BorderRadius.circular(6),
          ),
          child: Row(mainAxisAlignment: MainAxisAlignment.center, children: [
            const Text('Now (C):  ', style: TextStyle(fontSize: 16, color: Colors.white54)),
            Text('${_temp.toStringAsFixed(1)}',
              style: TextStyle(fontSize: 22, fontWeight: FontWeight.bold,
                color: _temp < 18 ? Colors.blue : (_temp > 35 ? Colors.red : Colors.white))),
          ]),
        ),
        const SizedBox(height: 14),

        _oledRow(-1, 'Status', tempStatus, Icons.info_outline, statusColor),
        const SizedBox(height: 8),
        _oledRow(-1, 'Heater', _heater ? 'ON' : 'OFF', Icons.whatshot,
          _heater ? Colors.red : Colors.white38),
      ]),
    );
  }

  // ============ PUMP Page ============
  Widget _pumpPage() {
    if (_sub == 0) {
      final cond = _pumpCond;
      Color condColor;
      switch (cond) {
        case 'Good':  condColor = Colors.green;  break;
        case 'Fair':  condColor = Colors.cyan;   break;
        case 'Dirty': condColor = Colors.orange; break;
        default:      condColor = Colors.red;    break;
      }
      return Padding(
        padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 20),
        child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
          const Text('PUMP', style: TextStyle(fontSize: 18, color: Colors.cyan, letterSpacing: 2)),
          const SizedBox(height: 20),
          _oledRow(0, 'Condition', cond, Icons.info_outline, condColor),
          const SizedBox(height: 12),
          _oledRow(1, 'Setting', '', Icons.settings, Colors.white54),
        ]),
      );
    } else {
      final items = ['P1out', 'P2into', 'End'];
      final icons = [Icons.water_drop, Icons.water, Icons.stop];
      final runningIdx = _pump - 1; // 0=P1out, 1=P2into
      return Column(children: [
        const SizedBox(height: 12),
        const Text('PUMP SETTING', style: TextStyle(fontSize: 18, color: Colors.cyan, letterSpacing: 2)),
        const SizedBox(height: 16),
        Expanded(child: ListView(
          shrinkWrap: true,
          children: List.generate(3, (i) {
            final running = (i == runningIdx);
            return _menuRow(i, _MenuItem(
              running ? '${items[i]} (ON)' : items[i],
              running ? Icons.play_circle : icons[i],
            ));
          }),
        )),
      ]);
    }
  }

  // ============ OLED-style row with cursor ============
  Widget _oledRow(int index, String label, String value, IconData icon, Color color) {
    final selected = _isSelected(index);
    return Row(children: [
      Text(selected ? '>' : ' ', style: TextStyle(fontSize: 16, color: selected ? Colors.cyan : Colors.transparent)),
      const SizedBox(width: 8),
      Icon(icon, size: 18, color: selected ? Colors.cyan : color.withOpacity(0.6)),
      const SizedBox(width: 8),
      Text('$label: ', style: TextStyle(fontSize: 15, color: selected ? Colors.cyan : Colors.white54)),
      Expanded(child: Text(value, textAlign: TextAlign.right,
        style: TextStyle(fontSize: 15, fontWeight: FontWeight.bold, color: color))),
    ]);
  }

  // ============ Menu List ============
  Widget _menuList(List<_MenuItem> items, {String? title}) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
        if (title != null) ...[
          Text(title, style: const TextStyle(fontSize: 18, color: Colors.cyan, letterSpacing: 2)),
          const SizedBox(height: 14),
        ],
        ...List.generate(items.length, (i) => _menuRow(i, items[i])),
      ]),
    );
  }

  Widget _menuRow(int index, _MenuItem item) {
    final selected = _isSelected(index);
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 3, horizontal: 16),
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 180),
        curve: Curves.easeOut,
        padding: EdgeInsets.symmetric(vertical: selected ? 10 : 7, horizontal: selected ? 14 : 10),
        decoration: BoxDecoration(
          color: selected ? Colors.cyan.withOpacity(0.12) : Colors.transparent,
          borderRadius: BorderRadius.circular(8),
          border: selected
              ? Border.all(color: Colors.cyan.withOpacity(0.35), width: 1)
              : Border.all(color: Colors.transparent, width: 1),
        ),
        child: Row(mainAxisSize: MainAxisSize.min, children: [
          Icon(item.icon, size: selected ? 22 : 18, color: selected ? Colors.cyan : Colors.white38),
          const SizedBox(width: 10),
          AnimatedDefaultTextStyle(
            duration: const Duration(milliseconds: 180),
            curve: Curves.easeOut,
            style: TextStyle(
              fontSize: selected ? 17 : 14,
              fontWeight: selected ? FontWeight.bold : FontWeight.normal,
              color: selected ? Colors.cyan : Colors.white70,
            ),
            child: Text(item.label),
          ),
          if (selected) ...[
            const SizedBox(width: 6),
            const Text('◀', style: TextStyle(fontSize: 12, color: Colors.cyan)),
          ],
        ]),
      ),
    );
  }

  Widget _runningBadge(String label, bool running) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 10),
      child: Row(mainAxisAlignment: MainAxisAlignment.center, children: [
        Container(
          width: 8, height: 8,
          decoration: BoxDecoration(shape: BoxShape.circle, color: running ? Colors.green : Colors.white24),
        ),
        const SizedBox(width: 6),
        Text(running ? '$label Running' : '$label Stopped',
          style: TextStyle(fontSize: 13, color: running ? Colors.green : Colors.white38)),
      ]),
    );
  }

  // ============ D-Pad ============
  Widget _dpad() {
    return Column(
      children: [
        GestureDetector(
          onTapDown: (_) => _send('u'),
          child: SizedBox(height: 64, width: 64, child: FloatingActionButton(heroTag: 'u', onPressed: null, child: const Icon(Icons.keyboard_arrow_up, size: 36))),
        ),
        const SizedBox(height: 4),
        Stack(alignment: Alignment.center, children: [
          Row(mainAxisAlignment: MainAxisAlignment.center, children: [
            GestureDetector(
              onTapDown: (_) => _send('b'),
              child: SizedBox(height: 56, width: 56, child: FloatingActionButton(heroTag: 'b', onPressed: null, child: const Icon(Icons.keyboard_arrow_left, size: 32))),
            ),
            const SizedBox(width: 56),
            GestureDetector(
              onTapDown: (_) => _send('e'),
              child: SizedBox(height: 56, width: 56, child: FloatingActionButton(heroTag: 'e', onPressed: null, child: const Icon(Icons.keyboard_arrow_right, size: 32))),
            ),
          ]),
          GestureDetector(
            onTapDown: (_) => _send('0'),
            child: AnimatedContainer(
              duration: const Duration(milliseconds: 150),
              width: 32, height: 32,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: _nullFlash > 0 ? Colors.red.withOpacity(0.7) : Colors.white24,
              ),
              child: Center(child: Text('N', style: TextStyle(fontSize: 11, color: _nullFlash > 0 ? Colors.white : Colors.white54))),
            ),
          ),
        ]),
        const SizedBox(height: 4),
        GestureDetector(
          onTapDown: (_) => _send('d'),
          child: SizedBox(height: 64, width: 64, child: FloatingActionButton(heroTag: 'd', onPressed: null, child: const Icon(Icons.keyboard_arrow_down, size: 36))),
        ),
      ],
    );
  }
}

class _MenuItem {
  final String label;
  final IconData icon;
  const _MenuItem(this.label, this.icon);
}
