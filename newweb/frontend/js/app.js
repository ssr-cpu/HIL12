(() => {
  const STATES = ['POWER_OFF', 'INIT', 'SELF_TEST', 'STANDBY', 'RUN', 'FAULT', 'RECOVERY'];
  const HIDDEN_SIGNALS = ['ecu_state', 'ecu_fault_latched'];

  const el = (id) => document.getElementById(id);
  let state = null;
  let chartSignal = '';

  const DEMO_SCRIPT = `TEST power_up_normal
SET throttle_position 20
WAIT 300
ASSERT engine_speed GT 800 1
ASSERT battery_voltage EQ 12.5 0.01
END

TEST fault_stuck
SET throttle_position 30
WAIT 300
FAULT engine_speed STUCK 1500 100
WAIT 20
ASSERT engine_speed EQ 1500 1
WAIT 120
ASSERT engine_speed GT 800 1
RESET
END
`;

  const FULL_SCRIPT = `TEST power_off_and_restart
SET throttle_position 20
WAIT 300
ASSERT ecu_state EQ RUN
POWER OFF
ASSERT ecu_state EQ POWER_OFF
ASSERT engine_speed EQ 0 0.01
POWER ON
ASSERT ecu_state EQ INIT
SET throttle_position 20
WAIT 300
ASSERT ecu_state EQ RUN
ASSERT engine_speed GT 800 1
END

TEST comm_timeout_recoverable
SET throttle_position 20
WAIT 300
ASSERT ecu_state EQ RUN
FAULT ECU COMM
ASSERT ecu_state EQ FAULT
WAIT 20
ASSERT engine_speed EQ 0 0.01
FAULT ECU CLEAR
ASSERT ecu_state EQ RECOVERY
WAIT 300
ASSERT ecu_state EQ RUN
END

TEST ecu_latched_fault
SET throttle_position 20
WAIT 300
ASSERT ecu_state EQ RUN
FAULT ECU LATCH
ASSERT ecu_state EQ FAULT
ASSERT ecu_fault_latched EQ 1 0.01
WAIT 1000
ASSERT ecu_state EQ FAULT
ASSERT engine_speed EQ 0 0.01
FAULT ECU CLEAR
ASSERT ecu_state EQ RECOVERY
ASSERT ecu_fault_latched EQ 0 0.01
END
`;

  const GUIDE = [
    {
      cat: '必做功能',
      items: [
        { n: '1. 虚拟 ECU 状态机', how: '试验台：POWER ON → 连续 WAIT，看状态链走到 RUN', tab: 'bench' },
        { n: '1. 周期任务', how: 'RUN 状态下 engine_speed / vehicle_speed 随油门自动变化', tab: 'bench' },
        { n: '1. ≥8 路信号', how: '试验台信号表，实际 12 路', tab: 'bench' },
        { n: '2. 总线帧队列/丢失/延迟/篡改', how: '试验台·虚拟总线：设丢失率/延迟/篡改后 WAIT，看四计数器', tab: 'bench' },
        { n: '3. 脚本 SET/WAIT/FAULT/ASSERT/RESET', how: '脚本执行页载入示例并运行，看每步 PASS/FAIL', tab: 'script' },
        { n: '4. CSV 记录 + 按时间/信号查询', how: '数据记录页选信号查询；data/out/run.csv 落盘', tab: 'data' },
        { n: '5. 用例级 + 步骤级报告', how: '报告页读取最近一次报告', tab: 'report' },
        { n: '6. 配置/错误码/日志/构建', how: 'default_config.ini、hil_common.h、后端窗口日志、start.bat 一键构建', tab: 'bench' }
      ]
    },
    {
      cat: '必测场景',
      items: [
        { n: '1. 正常上电和完整业务流程', how: '脚本执行页"载入完整套件"，power_off_and_restart 用例', tab: 'script' },
        { n: '2. 启动/通信超时及断电', how: '试验台：POWER OFF/ON、FAULT ECU COMM', tab: 'bench' },
        { n: '3. 信号边界、传感器断线', how: 'SET 到 0/8000 看边界；注入 OPEN_CIRCUIT 看开路', tab: 'bench' },
        { n: '4. 报文丢失/延迟/篡改', how: '试验台·虚拟总线', tab: 'bench' },
        { n: '5. 可恢复与锁存故障', how: 'FAULT ECU COMM(可恢复) vs LATCH(锁存不自愈)', tab: 'bench' },
        { n: '6. 错误脚本/日志失败/长时间运行', how: '脚本写 BOGUS 报错；WAIT 10000 长跑', tab: 'script' }
      ]
    }
  ];

  function toast(message, bad) {
    const node = el('toast');
    node.textContent = message;
    node.className = 'toast show' + (bad ? ' bad' : '');
    setTimeout(() => { node.className = 'toast'; }, 2200);
  }

  function visibleSignals() {
    return (state.signals || []).filter((s) => HIDDEN_SIGNALS.indexOf(s.name) < 0);
  }

  function fillSelect(node, names, selected) {
    const previous = selected || node.value;
    node.innerHTML = names
      .map((n) => '<option value="' + n + '">' + n + '</option>')
      .join('');
    if (names.indexOf(previous) >= 0) {
      node.value = previous;
    }
  }

  function renderStateChain() {
    const current = state.ecu_state;
    el('state-chain').innerHTML = STATES.map((name) => {
      let cls = 'state-node';
      if (name === current) {
        cls += name === 'FAULT' ? ' fault' : ' on';
      }
      return '<span class="' + cls + '">' + name + '</span>';
    }).join('');
  }

  function renderSignals() {
    const faultedSignals = {};
    (state.faults || []).forEach((f) => {
      if (f.active) faultedSignals[f.signal] = f.type;
    });
    const rows = visibleSignals().map((s) => {
      let status;
      let rowClass = '';
      if (!s.valid) {
        status = '<span class="pill bad">' + (s.open ? '开路' : '无效') + '</span>';
      } else if (faultedSignals[s.name]) {
        status = '<span class="pill warn">故障:' + faultedSignals[s.name] + '</span>';
        rowClass = ' class="faulted"';
      } else {
        status = '<span class="pill ok">正常</span>';
      }
      return '<tr' + rowClass + '><td>' + s.name + '</td><td class="num">' + s.value +
        '</td><td>' + s.unit + '</td><td>' + status + '</td></tr>';
    });
    el('signal-table').querySelector('tbody').innerHTML = rows.join('');
    el('signal-count').textContent = String(state.signals.length);
  }

  function renderFaults() {
    const rows = (state.faults || []).map((f) => {
      const active = f.active
        ? '<span class="pill bad">生效</span>'
        : '<span class="pill muted">未生效</span>';
      return '<tr><td>' + f.signal + '</td><td>' + f.type + '</td><td class="num">' +
        f.value + '</td><td class="num">' + f.start_ms + '</td><td class="num">' +
        (f.end_ms || '∞') + '</td><td>' + active + '</td></tr>';
    });
    el('fault-table').querySelector('tbody').innerHTML =
      rows.join('') || '<tr><td colspan="6" style="color:#6b7280">当前没有活动故障</td></tr>';
  }

  function renderBus() {
    const bus = state.bus;
    el('bus-capacity').textContent = bus.capacity;
    el('bus-queued').textContent = bus.queued;
    el('bus-published').textContent = bus.published;
    el('bus-delivered').textContent = bus.delivered;
    el('bus-lost').textContent = bus.lost;
    el('bus-tampered').textContent = bus.tampered;
    el('stat-bus').textContent = bus.delivered + ' / ' + bus.published;
    el('bus-loss').value = bus.loss_percent;
    el('bus-delay').value = bus.delay_ms;
    el('bus-tamper').value = bus.tamper ? 'true' : 'false';
  }

  function render() {
    if (!state) return;
    el('stat-time').textContent = state.time_ms + ' ms';
    el('stat-state').textContent = state.ecu_state;
    el('stat-latched').textContent = state.latched ? '是' : '否';
    el('kv-power').textContent = state.power_requested ? '是' : '否';
    el('kv-comm').textContent = state.comm_fault ? '存在' : '无';
    el('kv-recovery').textContent = state.recovery_attempts;
    renderStateChain();
    renderSignals();
    renderFaults();
    renderBus();
  }

  async function refresh() {
    try {
      state = await HilApi.state();
      render();
    } catch (error) {
      toast('无法连接后端：' + error.message, true);
    }
  }

  async function call(action, label) {
    try {
      state = await action();
      render();
      if (label) toast(label);
    } catch (error) {
      toast(error.message, true);
    }
  }

  function drawChart(points, signal) {
    const canvas = el('chart');
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;
    ctx.clearRect(0, 0, w, h);
    ctx.fillStyle = '#fff';
    ctx.fillRect(0, 0, w, h);

    if (!points.length) {
      ctx.fillStyle = '#6b7280';
      ctx.font = '13px sans-serif';
      ctx.fillText('暂无采样，先执行一次 WAIT', 16, h / 2);
      return;
    }

    let min = Infinity;
    let max = -Infinity;
    points.forEach((p) => {
      if (p.v < min) min = p.v;
      if (p.v > max) max = p.v;
    });
    if (min === max) {
      min -= 1;
      max += 1;
    }
    const pad = 40;
    const t0 = points[0].t;
    const t1 = points[points.length - 1].t || t0 + 1;
    const x = (t) => pad + ((t - t0) / (t1 - t0)) * (w - pad - 12);
    const y = (v) => h - pad - ((v - min) / (max - min)) * (h - pad - 20);

    ctx.strokeStyle = '#e3e6ea';
    ctx.lineWidth = 1;
    ctx.font = '11px sans-serif';
    ctx.fillStyle = '#6b7280';
    for (let i = 0; i <= 4; i++) {
      const value = min + ((max - min) * i) / 4;
      const yy = y(value);
      ctx.beginPath();
      ctx.moveTo(pad, yy);
      ctx.lineTo(w - 12, yy);
      ctx.stroke();
      ctx.fillText(value.toFixed(2), 4, yy + 4);
    }

    ctx.strokeStyle = '#2563eb';
    ctx.lineWidth = 1.8;
    ctx.beginPath();
    points.forEach((p, i) => {
      if (i === 0) ctx.moveTo(x(p.t), y(p.v));
      else ctx.lineTo(x(p.t), y(p.v));
    });
    ctx.stroke();

    ctx.fillStyle = '#6b7280';
    ctx.fillText(t0 + ' ms', pad, h - 12);
    ctx.fillText(t1 + ' ms', w - 90, h - 12);
    ctx.fillStyle = '#1f2328';
    ctx.fillText(signal, w - 12 - ctx.measureText(signal).width, 18);
  }

  async function refreshChart() {
    const signal = el('chart-signal').value || chartSignal;
    if (!signal) return;
    chartSignal = signal;
    try {
      const result = await HilApi.history(signal, 0, 4294967295);
      const points = result.history.points;
      drawChart(points, signal);
      el('chart-hint').textContent =
        '采样点：' + points.length + '（步长 ' + result.history.stride + '）';
    } catch (error) {
      toast(error.message, true);
    }
  }

  function renderStepTable(tableId, steps, withMessage) {
    const rows = steps.map((s) => {
      const cls = s.status === 'PASS' ? 'ok' : s.status === 'FAIL' ? 'bad' : 'warn';
      const cells = [
        '<td>' + s.case + '</td>',
        '<td class="num">' + s.line + '</td>',
        '<td>' + s.type + '</td>',
        '<td>' + s.target + '</td>',
        '<td>' + s.expected + '</td>',
        '<td>' + s.actual + '</td>',
        '<td><span class="pill ' + cls + '">' + s.status + '</span></td>'
      ];
      if (withMessage) cells.push('<td>' + s.message + '</td>');
      return '<tr>' + cells.join('') + '</tr>';
    });
    el(tableId).querySelector('tbody').innerHTML = rows.join('');
  }

  function renderSummary(nodeId, summary, allPassed) {
    const chips = [
      '<span class="chip">步骤 ' + summary.steps + '</span>',
      '<span class="chip ok">通过 ' + summary.passed + '</span>',
      '<span class="chip ' + (summary.failed ? 'bad' : '') + '">失败 ' + summary.failed + '</span>',
      '<span class="chip">跳过 ' + summary.skipped + '</span>',
      '<span class="chip ' + (summary.errors ? 'bad' : '') + '">错误 ' + summary.errors + '</span>'
    ];
    if (allPassed !== undefined) {
      chips.push('<span class="chip ' + (allPassed ? 'ok' : 'bad') + '">' +
        (allPassed ? '全部通过' : '存在失败') + '</span>');
    }
    el(nodeId).innerHTML = chips.join('');
  }

  function renderGuide() {
    const html = GUIDE.map((group) => {
      const rows = group.items.map((item) =>
        '<tr><td style="white-space:nowrap;font-weight:500">' + item.n + '</td>' +
        '<td style="color:#6b7280">' + item.how + '</td>' +
        '<td><button class="btn ghost" data-goto="' + item.tab + '">去验证</button></td></tr>'
      ).join('');
      return '<h3 style="font-size:13px;margin:14px 0 6px;color:#2563eb">' +
        group.cat + '</h3><table class="table">' +
        '<thead><tr><th style="width:220px">需求项</th><th>怎么测</th><th style="width:100px"></th></tr></thead>' +
        '<tbody>' + rows + '</tbody></table>';
    }).join('');
    el('guide-list').innerHTML = html;
    el('guide-list').querySelectorAll('[data-goto]').forEach((btn) => {
      btn.addEventListener('click', () => {
        document.querySelectorAll('.tab').forEach((t) => t.classList.remove('active'));
        document.querySelectorAll('.panel').forEach((p) => p.classList.remove('active'));
        document.querySelector('.tab[data-tab="' + btn.dataset.goto + '"]').classList.add('active');
        el('tab-' + btn.dataset.goto).classList.add('active');
      });
    });
  }

  function bindTabs() {
    document.querySelectorAll('.tab').forEach((tab) => {
      tab.addEventListener('click', () => {
        document.querySelectorAll('.tab').forEach((t) => t.classList.remove('active'));
        document.querySelectorAll('.panel').forEach((p) => p.classList.remove('active'));
        tab.classList.add('active');
        el('tab-' + tab.dataset.tab).classList.add('active');
      });
    });
  }

  function bindControls() {
    el('btn-set').addEventListener('click', () => {
      call(() => HilApi.set(el('set-signal').value, el('set-value').value), '已设置信号');
    });
    el('btn-wait').addEventListener('click', async () => {
      await call(() => HilApi.wait(el('wait-ms').value), '时间已推进');
      if (state) await refreshChart();
    });
    el('btn-reset').addEventListener('click', () => {
      call(() => HilApi.reset(), '系统已复位');
    });
    el('btn-clear-faults').addEventListener('click', () => {
      call(() => HilApi.clearFaults(), '信号故障已清除');
    });
    el('btn-inject').addEventListener('click', () => {
      call(() => HilApi.fault(
        el('fault-signal').value,
        el('fault-type').value,
        el('fault-value').value,
        el('fault-duration').value
      ), '故障已注入');
    });
    document.querySelectorAll('[data-ecufault]').forEach((btn) => {
      btn.addEventListener('click', () => {
        call(() => HilApi.ecuFault(btn.dataset.ecufault), 'ECU 故障动作：' + btn.dataset.ecufault);
      });
    });
    document.querySelectorAll('[data-power]').forEach((btn) => {
      btn.addEventListener('click', () => {
        call(() => HilApi.power(btn.dataset.power), '电源：' + btn.dataset.power);
      });
    });
    el('btn-assert').addEventListener('click', async () => {
      try {
        const result = await HilApi.assert(
          el('assert-signal').value,
          el('assert-op').value,
          el('assert-value').value,
          el('assert-tol').value
        );
        state = result;
        render();
        const a = result.assert;
        el('assert-result').innerHTML = a.passed
          ? '<span class="pill ok">PASS</span> ' + a.message
          : '<span class="pill bad">FAIL</span> ' + a.message;
      } catch (error) {
        toast(error.message, true);
      }
    });
    el('btn-bus-apply').addEventListener('click', () => {
      call(() => HilApi.config(
        el('bus-loss').value,
        el('bus-delay').value,
        el('bus-tamper').value
      ), '总线参数已应用');
    });
    el('btn-refresh-chart').addEventListener('click', refreshChart);
    el('chart-signal').addEventListener('change', refreshChart);

    el('btn-run-script').addEventListener('click', async () => {
      const button = el('btn-run-script');
      button.disabled = true;
      button.textContent = '运行中…';
      try {
        const result = await HilApi.runScript(el('script-text').value);
        renderSummary('script-summary', result.summary, result.all_passed);
        renderStepTable('script-table', result.steps, false);
        toast('脚本执行完成');
      } catch (error) {
        toast(error.message, true);
      } finally {
        button.disabled = false;
        button.textContent = '运行脚本';
      }
    });
    el('btn-load-demo').addEventListener('click', () => {
      el('script-text').value = DEMO_SCRIPT;
    });
    el('btn-load-full').addEventListener('click', () => {
      el('script-text').value = FULL_SCRIPT;
    });
    el('btn-load-report').addEventListener('click', async () => {
      try {
        const result = await HilApi.report();
        renderSummary('report-summary', result.summary, result.failed === 0 && result.errors === 0);
        renderStepTable('report-table', result.steps, true);
      } catch (error) {
        toast(error.message, true);
      }
    });
    el('btn-download-csv').addEventListener('click', () => {
      window.open('/api/state', '_blank');
      toast('CSV 位于后端 data/out 目录');
    });
    el('btn-query').addEventListener('click', async () => {
      try {
        const result = await HilApi.history(
          el('query-signal').value,
          el('query-from').value,
          el('query-to').value
        );
        const rows = result.history.points.map((p) =>
          '<tr><td class="num">' + p.t + '</td><td class="num">' + p.v + '</td></tr>');
        el('query-table').querySelector('tbody').innerHTML =
          rows.join('') || '<tr><td colspan="2" style="color:#6b7280">该时间窗内没有数据</td></tr>';
        el('query-hint').textContent =
          '命中 ' + result.history.points.length + ' 条记录（信号 ' +
          result.history.signal + '，' + result.history.from_ms + ' – ' +
          result.history.to_ms + ' ms）';
      } catch (error) {
        toast(error.message, true);
      }
    });
  }

  async function bootstrap() {
    bindTabs();
    bindControls();
    renderGuide();
    el('script-text').value = DEMO_SCRIPT;
    await refresh();
    const names = visibleSignals().map((s) => s.name);
    fillSelect(el('set-signal'), names);
    fillSelect(el('fault-signal'), names);
    fillSelect(el('chart-signal'), names);
    fillSelect(el('query-signal'), names);
    fillSelect(el('assert-signal'), (state.signals || []).map((s) => s.name));
    chartSignal = names[0];
    await refreshChart();
  }

  bootstrap();
})();
