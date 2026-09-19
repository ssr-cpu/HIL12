class VirtualHilApi {
  async get(path, params) {
    const url = new URL(path, window.location.origin);
    Object.entries(params || {}).forEach(([key, value]) => {
      url.searchParams.set(key, String(value));
    });
    const response = await fetch(url.toString());
    const data = await response.json();
    return { status: response.status, data };
  }

  run(params) {
    return this.get("/api/run", params);
  }

  query(params) {
    return this.get("/api/query", params);
  }

  file(params) {
    return this.get("/api/file", params);
  }
}

window.virtualHilApi = new VirtualHilApi();
