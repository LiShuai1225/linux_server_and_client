# linux_server_and_client
# tcp直接改成udp的版本，做了一些优化
# 包含了alone文件夹，做细节演示的单个客户端
------------------------------------------------
### 已知bug：
##  1.在vmware的nat网卡上，会出现连接非常卡的情况
------------------------------------------------
##  1.搞定了单客户演示的alone文件夹，加入了脚本控制vlc发送视频流
##  2.已经可以飞快的连接了
##  3.添加了显示窗口。先运行read，再运行server5 。若read不能运行，则先运行write建立fifo，关闭后再运行read
------------------------------------------------
### 下一步要做的：
------------------------------------------------
### 需要解决的：
####	1.在客户端强关后会有莫名数据包发来
####	2.close(socket)那里需要验证socket存在于否
#### 	3.服务端关闭某个socket，需要验证
####	4.完成整体的退出
####	5.epoll修改标识符，将发送操作加入下一轮处理，实现真正的异步
------------------------------------------------

