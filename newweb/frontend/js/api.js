const HilApi = (() => {
  const BASE = '';

  async function get(path, params) {
    const query = new URLSearchParams(params || {}).toString();
    const url = BASE + path + (query ? '?' + query : '');
    const response = await fetch(url, { cache: 'no-store' });
    return response.json();
  }

  async function post(path, params) {
    const body = new URLSearchParams(params || {}).toString();
    const response = await fetch(BASE + path, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body
    });
    return response.json();
  }

  function unwrap(result) {
    if (!result || result.ok !== true) {
      throw new Error((result && result.error) || '请求失败');
    }
    return result;
  }

  return {
    state: async () => unwrap(await get('/api/state')),
    set: (signal, value) => get('/api/set', { signal, value }).then(unwrap),
    wait: (ms) => get('/api/wait', { ms }).then(unwrap),
    fault: (signal, type, value, duration) =>
      get('/api/fault', { signal, type, value, duration }).then(unwrap),
    clearFaults: () => get('/api/fault/clear').then(unwrap),
    ecuFault: (action) => get('/api/ecufault', { action }).then(unwrap),
    power: (state) => get('/api/power', { state }).then(unwrap),
    reset: () => get('/api/reset').then(unwrap),
    assert: (signal, op, value, tol) =>
      get('/api/assert', { signal, op, value, tol }).then(unwrap),
    history: (signal, from, to) =>
      get('/api/history', { signal, from, to }).then(unwrap),
    runScript: (text) => post('/api/script', { script: text }).then(unwrap),
    report: () => get('/api/report').then(unwrap),
    config: (loss, delay, tamper) =>
      get('/api/config', { loss, delay, tamper }).then(unwrap)
  };
})();
