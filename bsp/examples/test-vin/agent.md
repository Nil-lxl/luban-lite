# 目标：
重新设计一个更通用的test-vin示例

# 要求：
- 支持DVP等这一类的video in设备，比如将来可能新增CSI接口
- 支持多通道，为了这个可能要给MPP新增一套接口（当前的mpp_vin_sel_ch()做法存在多线程安全隐患）
- 支持拼接模式（stitch mode）
- 要适配当前RTOS SDK的MPP、DVP driver接口
- 总体思路，可以参考Linux版本test-vin，源码位于上层目录的test-vin_only_for_linux ，其中用到了DRM、V4L2等接口，需要找到RTOS环境的替代方案。最好和Linux版本test-vin的命令行参数相似

# 输出：
另外创建一个示例test-vin，添加到SDK中，并检查编译情况
