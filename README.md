# Camera-of-LiaoNing-Robocon-2017
Golf ball recognition in LiaoNing robot contest 2017



### 存在问题

1. 重新调参数（有空的话整好滑动条调参）
2. 两条线卡球的最大距离范围（一条卡上边缘，一条卡球心）
3. VIDIOC_DQBUF no such device（可能是供电问题或摄像头松动，尝试通过软件方式解决）
4. raspberry DMA/FPU
5. 修改对于向量的使用，使用at调用向量元素
6. 车前方不是绿色场地发送特殊信息


