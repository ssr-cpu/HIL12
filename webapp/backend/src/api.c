#include "web_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WEB_POPEN _popen
#define WEB_PCLOSE _pclose
#else
#define WEB_POPEN popen
#define WEB_PCLOSE pclose
#endif

static int run_command(const char *command, char *output, size_t output_size,
                       int *exit_code)
{
    FILE *pipe;
    size_t used = 0U;
    int ch;
    if (output == NULL || output_size == 0U || exit_code == NULL) {
        return -1;
    }
    output[0] = '\0';
    pipe = WEB_POPEN(command, "r");
    if (pipe == NULL) {
        return -1;
    }
    while ((ch = fgetc(pipe)) != EOF && used + 1U < output_size) {
        output[used++] = (char)ch;
    }
    output[used] = '\0';
    *exit_code = WEB_PCLOSE(pipe);
#if defined(_WIN32)
    *exit_code = *exit_code == 0 ? 0 : *exit_code;
#else
    *exit_code = *exit_code == 0 ? 0 : *exit_code;
#endif
    return 0;
}

static const char *param_or_default(const web_request_t *request,
                                    const char *key,
                                    const char *default_value,
                                    char *buffer, size_t buffer_size)
{
    if (web_query_get(request->query, key, buffer, buffer_size)) {
        return buffer;
    }
    return default_value;
}

static int json_response(char *body, size_t body_size, bool ok,
                         const char *error, const char *output,
                         int exit_code)
{
    char escaped[WEB_MAX_RESPONSE];
    web_json_escape(output, escaped, sizeof(escaped));
    if (error == NULL) {
        error = "";
    }
    (void)snprintf(body, body_size,
                   "{\"ok\":%s,\"exit_code\":%d,\"output\":\"%s\","
                   "\"error\":\"%s\"}",
                   ok ? "true" : "false", exit_code, escaped, error);
    return ok ? 200 : 500;
}

static int api_status(char *body, size_t body_size)
{
    (void)snprintf(body, body_size,
                   "{\"ok\":true,\"name\":\"virtual_hil_web\","
                   "\"backend\":\"C17\",\"port\":%d}",
                   WEB_DEFAULT_PORT);
    return 200;
}

static int api_run(const web_request_t *request, char *body, size_t body_size)
{
    char config[WEB_MAX_PATH] = "";
    char signals[WEB_MAX_PATH] = "";
    char script[WEB_MAX_PATH] = "";
    char out[WEB_MAX_PATH] = "";
    char lang[WEB_MAX_PATH] = "";
    const char *config_value;
    const char *signals_value;
    const char *script_value;
    const char *out_value;
    const char *lang_value;
    char command[2048];
    char output[WEB_MAX_RESPONSE - 2048U];
    int exit_code = 0;
    int written;

    config_value =
        param_or_default(request, "config",
                         "../virtual_hil/examples/default_config.ini",
                         config, sizeof(config));
    signals_value =
        param_or_default(request, "signals",
                         "../virtual_hil/examples/signals.csv",
                         signals, sizeof(signals));
    script_value = param_or_default(
        request, "script", "../virtual_hil/examples/demo.hil", script,
        sizeof(script));
    out_value =
        param_or_default(request, "out", "data/out", out, sizeof(out));
    lang_value = param_or_default(request, "lang", "zh", lang, sizeof(lang));

    if (!web_is_safe_command_arg(config_value) ||
        !web_is_safe_command_arg(signals_value) ||
        !web_is_safe_command_arg(script_value) ||
        !web_is_safe_command_arg(out_value) ||
        (strcmp(lang_value, "zh") != 0 && strcmp(lang_value, "en") != 0)) {
        return json_response(body, body_size, false,
                             "invalid or unsafe parameter", "", -1);
    }

    written = snprintf(command, sizeof(command),
                       "..\\virtual_hil\\build\\hil.exe run --config \"%s\" "
                       "--signals \"%s\" --script \"%s\" --out \"%s\" "
                       "--lang %s 2>&1",
                       config_value, signals_value, script_value, out_value,
                       lang_value);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        return json_response(body, body_size, false,
                             "command is too long", "", -1);
    }
    if (run_command(command, output, sizeof(output), &exit_code) != 0) {
        return json_response(body, body_size, false,
                             "failed to execute hil.exe", "", -1);
    }
    return json_response(body, body_size, exit_code == 0, NULL, output,
                         exit_code);
}

static int api_query(const web_request_t *request, char *body, size_t body_size)
{
    char csv[WEB_MAX_PATH] = "";
    char signal[WEB_MAX_PATH] = "";
    char from_text[WEB_MAX_PATH] = "";
    char to_text[WEB_MAX_PATH] = "";
    const char *csv_value =
        param_or_default(request, "csv", "data/out/run.csv", csv, sizeof(csv));
    const char *signal_value =
        param_or_default(request, "signal", "engine_speed", signal,
                         sizeof(signal));
    const char *from_value =
        param_or_default(request, "from", "0", from_text, sizeof(from_text));
    const char *to_value =
        param_or_default(request, "to", "4294967295", to_text,
                         sizeof(to_text));
    char command[2048];
    char output[WEB_MAX_RESPONSE - 2048U];
    int exit_code = 0;
    int written;

    if (!web_is_safe_command_arg(csv_value) ||
        !web_is_safe_command_arg(signal_value) ||
        !web_is_safe_command_arg(from_value) ||
        !web_is_safe_command_arg(to_value)) {
        return json_response(body, body_size, false,
                             "invalid or unsafe parameter", "", -1);
    }
    written = snprintf(command, sizeof(command),
                       "..\\virtual_hil\\build\\hil.exe query --csv \"%s\" --signal "
                       "\"%s\" --from %s --to %s 2>&1",
                       csv_value, signal_value, from_value, to_value);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        return json_response(body, body_size, false,
                             "command is too long", "", -1);
    }
    if (run_command(command, output, sizeof(output), &exit_code) != 0) {
        return json_response(body, body_size, false,
                             "failed to execute hil.exe", "", -1);
    }
    return json_response(body, body_size, exit_code == 0, NULL, output,
                         exit_code);
}

static int api_file(const web_request_t *request, char *body, size_t body_size)
{
    char path[WEB_MAX_PATH] = "";
    const char *path_value =
        param_or_default(request, "path", "data/out/report.md", path,
                         sizeof(path));
    char file_content[WEB_MAX_RESPONSE];
    char escaped[WEB_MAX_RESPONSE];
    long length;
    if (!web_is_safe_command_arg(path_value)) {
        return json_response(body, body_size, false,
                             "invalid or unsafe path", "", -1);
    }
    length = web_read_file(path_value, file_content, sizeof(file_content));
    if (length < 0L) {
        return json_response(body, body_size, false,
                             "file not found or too large", "", -1);
    }
    web_json_escape(file_content, escaped, sizeof(escaped));
    (void)snprintf(body, body_size, "{\"ok\":true,\"path\":\"%s\","
                                     "\"content\":\"%s\"}",
                   path_value, escaped);
    return 200;
}

int web_api_dispatch(const web_request_t *request, char *body, size_t body_size)
{
    if (request == NULL || body == NULL) {
        return -1;
    }
    if (strcmp(request->path, "/api/status") == 0) {
        return api_status(body, body_size);
    }
    if (strcmp(request->path, "/api/run") == 0) {
        return api_run(request, body, body_size);
    }
    if (strcmp(request->path, "/api/query") == 0) {
        return api_query(request, body, body_size);
    }
    if (strcmp(request->path, "/api/file") == 0) {
        return api_file(request, body, body_size);
    }
    (void)snprintf(body, body_size, "{\"ok\":false,\"error\":\"not found\"}");
    return 404;
}
