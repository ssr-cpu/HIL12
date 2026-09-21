function value(id) {
  return document.getElementById(id).value.trim();
}

function setOutput(text) {
  document.getElementById("output").textContent = text;
}

function resetDefaults() {
  document.getElementById("run-config").value =
    "../virtual_hil/examples/default_config.ini";
  document.getElementById("run-signals").value =
    "../virtual_hil/examples/signals.csv";
  document.getElementById("run-script").value =
    "../virtual_hil/examples/demo.hil";
  document.getElementById("run-out").value = "data/out";
  document.getElementById("run-lang").value = "zh";
  document.getElementById("query-csv").value = "data/out/run.csv";
  document.getElementById("query-signal").value = "engine_speed";
  document.getElementById("query-from").value = "0";
  document.getElementById("query-to").value = "4294967295";
  document.getElementById("file-path").value = "data/out/report.md";
  setOutput("已恢复默认示例路径。现在可以直接点击“运行测试”。");
}

async function runTest() {
  setOutput("正在运行测试，请稍候...\n\n第一次运行可能等待 1 至 2 秒。");
  const button = document.getElementById("btn-run");
  button.disabled = true;
  button.textContent = "运行中...";
  const result = await window.virtualHilApi.run({
    config: value("run-config"),
    signals: value("run-signals"),
    script: value("run-script"),
    out: value("run-out"),
    lang: value("run-lang")
  });
  button.disabled = false;
  button.textContent = "运行测试";
  const body = result.data;
  if (!body.ok) {
    setOutput(`运行失败：${body.error || "未知错误"}\n\n${body.output || ""}`);
    return;
  }
  setOutput(`${body.output || "运行完成。"}\n\n下一步：在“查看报告 / 文件”中点击“加载文件”。`);
}

async function querySignal() {
  setOutput("正在查询，请稍候...");
  const result = await window.virtualHilApi.query({
    csv: value("query-csv"),
    signal: value("query-signal"),
    from: value("query-from"),
    to: value("query-to")
  });
  const body = result.data;
  if (!body.ok) {
    setOutput(`查询失败：${body.error || "未知错误"}\n\n${body.output || ""}`);
    return;
  }
  setOutput(body.output || "查询完成，但没有返回内容。");
}

async function loadFile() {
  const path = value("file-path");
  setOutput(`正在加载 ${path} ...`);
  const result = await window.virtualHilApi.file({ path });
  const body = result.data;
  if (!body.ok) {
    setOutput(`加载失败：${body.error || "未知错误"}`);
    return;
  }
  setOutput(body.content || "(空文件)");
}

document.addEventListener("DOMContentLoaded", () => {
  document.getElementById("btn-run").addEventListener("click", runTest);
  document.getElementById("btn-query").addEventListener("click", querySignal);
  document.getElementById("btn-file").addEventListener("click", loadFile);
  document.getElementById("btn-reset-defaults").addEventListener(
    "click", resetDefaults);
});
