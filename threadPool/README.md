# 简易线程池

- 结构
  - main thread : submit task
  - worker thread : consume task
  - task queue : 临界区
- 待完成：
  - 接收worker thread执行任务结果
  - 线程池析构时如何回收资源，是否销毁未执行任务
  - 如何应对mutex被销毁情况（手动测试已经安全，不过tsan监测到有data race : 销毁cond时唤醒cond，合理怀疑是tsan bug）