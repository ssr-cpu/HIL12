# 角色 3：总线开发

## 职责

负责虚拟总线帧结构、帧队列、编码解码、CRC，以及帧丢失、延迟和篡改故障。

## 实现文件

- `include/hil_frame.h`、`src/hil_frame.c`
- `include/hil_bus.h`、`src/hil_bus.c`

## 帧模块

- `hil_frame_init()` 初始化帧。
- `hil_frame_calculate_crc()` / `hil_frame_refresh_crc()` / `hil_frame_is_crc_valid()` 提供 CRC 校验。
- `hil_frame_set_payload()` / `hil_frame_get_payload()` 安全读写 8 字节 payload。
- `hil_frame_encode_double()` / `hil_frame_decode_double()` 将物理值按比例/偏移编入 16 位小端原始值。
- `hil_frame_to_string()` / `hil_frame_from_string()` 文本序列化与反序列化。

## 总线模块

- 环形队列：`hil_bus_publish()`、`hil_bus_poll()`、`hil_bus_inject_frame()`。
- 队列满返回 `HIL_ERR_BUSY`。
- 帧丢失按百分比随机丢弃。
- 帧延迟按 `delay_ms` 推迟交付。
- 帧篡改翻转 payload 中一个 bit，并破坏 CRC。

## 错误码与所有权

- `hil_bus_t` 拥有队列内存，`hil_bus_deinit()` 负责释放。
- 帧对象按值传递，不涉及外部所有权。
- 未初始化队列返回 `HIL_ERR_RESOURCE`。

## 验证

- `frame_crc_and_payload`、`frame_encode_decode`、`frame_encode_overflow`、`frame_string_roundtrip`。
- `bus_publish_poll`、`bus_queue_full`、`bus_delay`、`bus_loss`、`bus_tamper`。
