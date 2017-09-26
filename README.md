# Camera-of-LiaoNing-Robocon-2017
Golf ball recognition in LiaoNing robot contest 2017



### 存在问题

1. 尝试canny+颜色分割方式检测球（去除反光影响）
   * 漫水填充 `floodFill()`
   * `copyTo()` & `Mask`
2. 重新调参数（有空的话整好滑动条调参）
3. 查形态学判断球的方法
4. 两条线卡球的最大距离范围（一条卡上边缘，一条卡球心）
5. VIDIOC_DQBUF no such device（可能是供电问题或摄像头松动，尝试通过软件方式解决）
6. raspberry DMA/FPU
7. 修改对于向量的使用，使用at调用向量元素

