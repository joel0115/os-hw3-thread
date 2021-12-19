# OS2021_Hw3_Template
* [Hw3 requirements](https://docs.google.com/presentation/d/1UFuPUwd17Hogh5Vp8GZbnrLRAddGvC1j/edit#slide=id.p3)

author F74082010 資訊系112 陳昭穎

## 資料結構
將 thread 的資訊(tid、state、name等)打包成 struct，

並且採用 queue 來維護每個 thread 的狀態。

在本次作業中，priority 與數字有 mapping 關係， 0 代表 H 、 1 代表 M 、 2 代表 L。

## parse json
使用 cJSON 來 parse init_threads.json

只要將 cJSON.c 與 cJSON.h 放進 project 資料夾底下，

並且include標頭檔、更改 makefile 讓它能夠被 link 即可。

## API
* int OS2021_ThreadCreate(char *job_name, char *p_function, char* priority, int cancel_mode)
  * malloc 一塊記憶體空間，並將對應的資訊填入，還無法確定值的資料就初始化成0或NULL。
  * 將初始化好的 thread enqueue 到對應 priority 的 ready queue 裡面
  * 最後會回傳 tid ， 如果失敗的話會回傳 -1。

* void OS2021_ThreadCancel(char *job_name)
  * 若 cancel mode 為 0 ， 則直接將名字為 job_name 的 thread 從 ready 或 waiting queue dequeue掉，

    並且放入 terminated queue ， 等待 reclaimer 將資源回收 ， 若要 cancel 的 thread 正在 running ，
    
    則在放入 terminated queue 以後，會進行 context switch ， 讓 dispatcher 選擇要讓哪個 thread 開始執行。

  * 若 cancel mode 為 1 ， 會通知該 thread 它該被取消了，此時會一直等到該 thread 跑到 canellation point ( OS2021_TestCancel() ) 才會放入 terminated queue ，
    並且執行 context switch 讓 dispatcher 選擇下一個 thread 。
    
* void OS2021_ThreadWaitEvent(int event_id)
  * 將 thread 放入對應的 waiting queue ， waiting queue 有 8 * 3 個 ， 而 waiting_queue[i][j] 裡面放的是 正在等待 event i ， 且 priority 為 j 的 threads。
  * 此 API 也會進行 context switch ， 讓 dispatcher 選擇下一個 thread。

* void OS2021_ThreadSetEvent(int event_id)
  * 會到 waiting_queue[ event_id ] 裡尋找 thread ， priority 最優先的 thread 會優先回到 ready queue。
  
* void OS2021_ThreadWaitTime(int msec)
  * 讓執行該 API 的 thread 從 ready queue 放到 waiting_for_time_queue ， 並且將它該等待的時間設為 10 * msec。
  * 當 timer 觸發 SIGALRM 時 ， 會將 waiting_for_time_queue 中的所有 thread 該等待的時間減10。
  * 當該等待的時間歸零時，會再讓該 thread 回到 ready queue 中。

* void OS2021_DeallocateThreadResource()
  * 用來釋放 terminated_queue 裡的 thread 的記憶體空間，此 API 交由 reclaimer 執行，因此要等待 reclaimer 被 dispatcher 挑選中時才會進行釋放資源的操作。

* void OS2021_TestCancel()
  * 即為 cancellation point ， 當 thread 執行到此 API 時，若被通知刪除的 flag 為 1 ， 則將它從 ready queue 移動到 terminated queue 等待釋放資源。

## Timer handler
本次作業中，timer 觸發 SIGALRM 以後會進行的動作如下：
* 將正在執行的 thread 的 time quantum 減10，若 time quantum 用完了，則將該 thread 的 priority 下降一個等級，並且讓 dispatcher 選擇下個 thread，
* 將狀態為 READY 的 thread 的 queueing time 加10， WAITING 的 thread 的 waiting time 加10。
* 將 waiting_for_time_queue 中等待時間已經用盡的 thread 從 thread 中拿掉。

     
