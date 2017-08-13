# Camera-of-LiaoNing-Robocon-2017
Golf ball recognition in LiaoNing robot contest 2017



### 存在问题

1. 创建滑动条方便进行调参
2. cmake管理工程
3. 黑球空缺填补
4. 开机时间过长报错VIDIOC_DQBUF no such device
5. 形状判定非球物体
6. 检测到车的前方不是绿色场地发0
7. 完成开关机及程序挂掉自动重启脚本
   * 开机自启未实现
   * 关机实现：配置GPIO为输入模式，读取电平值关机
8. 尝试HSV+BGR的颜色判定方式
9. 通过连通域y的最小值锁识别区间

