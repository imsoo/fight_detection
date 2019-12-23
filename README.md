### Overview
Real time Fight Detection Based on 2D Pose Estimation and RNN Action Recognition. 

This project is based on [darknet_server](https://github.com/imsoo/darknet_server). if you want to run this experiment take a look how to build [here](https://github.com/imsoo/darknet_server#how-to-build). 


| ```Fight Detection System Pipeline``` |
|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71320889-4e737b80-24f5-11ea-8aac-4b4a527c6e64.png" width="100%" height="45%">|

### Pose Estimation and Object Tracking
Made pipeline to get 2D pose time series data in video sequence.

In worker process, Pose Estimation is performed using ***OpenPose***. Input image pass through the Pose_detector, and get the people object which packed up people joint coordinates. People object serialized and send to sink process.

In Sink process, people object convert to person objects. and every person object are send to Tracker. Tracker receive person object and produces object identities Using ***SORT***(simple online and realtime tracking algorithm).

Finally, can get the joint time series data per each person. each person's Time series data is managed by queue container. so person object always maintain recent 32 frame.

| ```Tracking Pipeline``` | ```Time series data``` |
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71316697-6b895980-24b7-11ea-92a1-33ec0dcd996a.png" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71302619-b5f3d300-23f0-11ea-9ab0-36791dd9aa48.png" width="150%" height="30%">|

* #### Examples of Result (Pose Estimation and Object Tracking)

|<img src="https://user-images.githubusercontent.com/11255376/71260111-64f6c700-237d-11ea-918c-1e8d9f05d963.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71260228-b3a46100-237d-11ea-9116-b050841a760b.gif" width="150%" height="30%">|
|:---:|:---:|


### Collect action video
Collected Four Action type (Standing, Walking, Punching, Kicking) video.

Punching Action type video is a subset of the ***Berkeley Multimodal Human Action Database (MHAD) dataset***.
This video data is comprised of 12 subjects doing the punching actions for 5 repetitions, filmed from 4 angles. (http://tele-immersion.citris-uc.org/berkeley_mhad)

Others (Standing, Walking, Kicking) are subsets of the ***CMU Panoptic Dataset***. In Range of Motion videos(171204_pose3, 171204_pose5, 171204_pose6), cut out 3 action type. I recorded timestamp per action type and cut out the video using python script(util/concat.py). This video data is comprised of 13 subjects doing the three actions, filmed from 31 angles. (http://domedb.perception.cs.cmu.edu/index.html)

``` jsonc
0, 1, 10    // 0 : Standing, 1 : begin timestamp, 10: end timestamp
1, 11, 15   // 1 : Walking, 11 : begin timestamp, 15: end timestamp
3, 39, 46   // 3 : Kicking, 39 : begin timestamp, 46: end timestamp
```

* #### Examples of Dataset (Stand & Walk)
<table>
<tr><th><code>Stand (CMU Panoptic Dataset)</code></th><th><code>Walk (CMU Panoptic Dataset)</code></th>
<tr valign="middle">
<td>

|<img src="https://user-images.githubusercontent.com/11255376/71252302-f65b3e80-2367-11ea-8718-a25a0ac7f14b.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71252304-f6f3d500-2367-11ea-9b8d-f5eff5b5959c.gif" width="150%" height="30%">|
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71252305-f78c6b80-2367-11ea-868e-988a94d28bb7.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71252307-f8250200-2367-11ea-9ab0-b1fc29672555.gif" width="150%" height="30%">|
</td>
<td>

|<img src="https://user-images.githubusercontent.com/11255376/71253038-0aa03b00-236a-11ea-99ae-80cd7bd1a284.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71253041-0b38d180-236a-11ea-834e-04c4c96a1974.gif" width="150%" height="30%">|
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71253042-0bd16800-236a-11ea-9a15-a0183d3be629.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71253043-0d029500-236a-11ea-94c2-993b969bb57c.gif" width="150%" height="30%">|
</td>
</tr>
</table>

* #### Examples of Dataset (Punch & Kick)
<table>
<tr><th><code>Punch (Berkeley MHAD Dataset)</code></th><th><code>Kick (CMU Panoptic Dataset)</code></th>
<tr>
<td>

|<img src="https://user-images.githubusercontent.com/11255376/71253986-cc584b00-236c-11ea-90e7-ff0934ffe38e.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71254000-e2660b80-236c-11ea-8aee-fb874a2d5365.gif" width="150%" height="30%">|
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71254136-5f918080-236d-11ea-9730-3368f3d4b143.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71254138-602a1700-236d-11ea-8077-848ff1233b8c.gif" width="150%" height="30%">|

</td>
<td>

|<img src="https://user-images.githubusercontent.com/11255376/71253059-17bd2a00-236a-11ea-895f-ccfd3bf3fc14.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71253060-18ee5700-236a-11ea-9f23-e0b0e600d5f4.gif" width="150%" height="30%">|
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71253062-1986ed80-236a-11ea-9699-8c5a5d2670b4.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71253066-1ab81a80-236a-11ea-988e-e1f1cf301031.gif" width="150%" height="30%">|

</td>
</tr>
</table>

### Make training dataset
Put action video data to tracking pipeline and get joint time series data per each person. this results (joint position) are processed to feature vector.

* ***Angle*** : current frame joint angle
* ***ΔPoint*** : A distance of prior frame joint point and current frame
* ***ΔAngle*** : A change of prior frame joint angle and current frame.

* #### Examples of feature vector (***ΔPoint*** & ***ΔAngle***)

| ***ΔPoint*** | ***ΔAngle*** |
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71304150-f3af2680-2405-11ea-8837-7c741b358956.gif" width="130%" height="50%">|<img src="https://user-images.githubusercontent.com/11255376/71304149-f1e56300-2405-11ea-8699-acece9a11722.gif" width="130%" height="50%">|

* #### Overview of feature vector

<table>
<tr><th><code>Feature Vector</code></th><th><code>OpenPose COCO output format</code></th>
<tr>
<td width="66%">

| *IDX* | *0* | *1*  | *2* | *3* | *4* | *5* | *6* | *7* |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| ***Angle*** | 2-3 | 3-4 | 5-6 | 6-7 | 8-9 | 9-10 | 11-12 | 12-13 |
| ***ΔAngle*** | 2-3 | 3-4 | 5-6 | 6-7 | 8-9 | 9-10 | 11-12 | 12-13 |
| ***ΔPoint*** | 3 | 4 | 6 | 7 | 9 | 10 | 12 | 13 |

###### *※ 2 : RShoulder, 3 : RElbow, 4 : RWrist, 5 : LShoulder, 6 : LElbow, 7 : LWrist, 8 : RHip, 9 : RKnee, 10 : RAnkle, 11 : LHip, 12 : LKnee, 13 : LAnkle*
</td>
<td width="34%">

|<img src="https://user-images.githubusercontent.com/11255376/71308335-866bb780-243e-11ea-9593-e13d80b15059.png" width="100%" height="30%">|
|:---:|

</td>
</tr>
</table>

Finally get each frame feature vector and then make action training data which consist of 32 frames feature vector. training datas are overlapped by 26 frames. so we got the four type action data set. 
A summary of the dataset  is:
* Standing : 7474 (7474 : pose3_stand) * 32 frame
* Walking : 4213 (854 : pose3_walk, 3359 : pose6_walk) * 32 frame
* Punching : 2187 (1115 : mhad_punch. 1072 : mhad_punch_flip) * 32 frame
* Kicking : 4694 (2558 : pose3_kick, 2136 : pose6_kick) * 32 frame
* total : 18573 * 32 frames (https://drive.google.com/open?id=1ZNJDzQUjo2lDPwGoVkRLg77eA57dKUqx)

|<img src="https://user-images.githubusercontent.com/11255376/71316454-1f3c1a80-24b3-11ea-9096-94e8cdc7adac.png" width="100%" height="50%">|
|:---:|

### RNN Training and Result
The network used in this experiment is based on that of :
* Guillaume Chevalier, 'LSTMs for Human Activity Recognition, 2016' https://github.com/guillaume-chevalier/LSTM-Human-Activity-Recognition
* stuarteiffert, 'RNN-for-Human-Activity-Recognition-using-2D-Pose-Input' https://github.com/stuarteiffert/RNN-for-Human-Activity-Recognition-using-2D-Pose-Input

training was run for 300 epochs with a batch size of 1024. (weights/action.h5)

After training, To get action recognition result in real time, made action recognition pipeline. Sink process send each person's time series feature vector to action process as string. Action process put received data into RNN network and send back results of prediction. (0 : Standing, 1 : Walking, 2 : Punching, 3 : Kicking)

| ```Action Recognition Pipeline``` | ```RNN Model``` |
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71318418-ed877b80-24d3-11ea-993c-d776d8e980c4.png" width="100%" height="50%">|<img src="https://user-images.githubusercontent.com/11255376/71318651-c7afa600-24d6-11ea-8baa-23153316bee8.png" width="100%" height="50%">|


* #### Examples of Result (RNN Action Recognition)
| [```standing```](https://www.youtube.com/watch?v=Orc0Eq9bWOs) | [```Walking```](https://www.youtube.com/watch?v=Orc0Eq9bWOs) | [```Punching```](https://www.youtube.com/watch?v=kbgkeTTSau8) | [```Kicking```](https://www.youtube.com/watch?v=R1UWcG9N6tI) |
|:---:|:---:|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71256358-d6317c80-2373-11ea-8fd9-2ae1777c8a0f.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71256359-d6ca1300-2373-11ea-812a-babb3b5b2ad5.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71256361-d7fb4000-2373-11ea-8a17-26ce9f9dc8f5.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71256362-d893d680-2373-11ea-841c-ced2f1d4ba02.gif" width="150%" height="30%">|

### Fight Detection

This stage check that person who kick or punch is hitting someone. if some person has hit other, those people set enemy each other.
System count it as fight and then track them until they exist in frame.

| ```Fight Detection Pipeline``` |
|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71320794-cc368780-24f3-11ea-8928-8e920bc69f26.png" width="100%" height="50%">|

* #### Examples of Result
| [```Fighting Championship```](https://www.youtube.com/watch?v=cIhoK4cPbC4) | [```CCTV Video```](https://www.youtube.com/watch?v=stJPOb6zW7U) |
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71256826-54dae980-2375-11ea-808b-be89bfaea5c1.gif" width="150%" height="30%">|<img src="https://user-images.githubusercontent.com/11255376/71356085-809fde80-25c4-11ea-90a9-a63eef4d6629.gif" width="150%" height="30%">|

| [```Sparring video A```](https://www.youtube.com/watch?v=x0kJmieuFzI) | [```Sparring video B```](https://www.youtube.com/watch?v=x0kJmieuFzI) |
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71356417-821dd680-25c5-11ea-945a-d4dab5f9e1e0.gif" width="130%" height="60%">|<img src="https://user-images.githubusercontent.com/11255376/71356526-df198c80-25c5-11ea-98bf-11e4edf81430.gif" width="130%" height="60%">|

* #### Examples of Result (Failure case)
| [```Fake Person```](https://www.youtube.com/watch?v=kbgkeTTSau8)  | [```Small Person```](https://www.youtube.com/watch?v=aUdKzb4LGJI) | 
|:---:|:---:|
|<img src="https://user-images.githubusercontent.com/11255376/71256575-7daeaf00-2374-11ea-82dd-579a07788acc.gif" width="130%" height="50%">|<img src="https://user-images.githubusercontent.com/11255376/71257257-94ee9c00-2376-11ea-8940-659a7eae08b8.gif" width="130%" height="50%">|

### References
* #### Darknet : https://github.com/AlexeyAB/darknet
* #### OpenCV : https://github.com/opencv/opencv
* #### ZeroMQ : https://github.com/zeromq/libzmq
* #### json-c : https://github.com/json-c/json-c 
* #### openpose-darknet : https://github.com/lincolnhard/openpose-darknet
* #### sort-cpp : https://github.com/mcximing/sort-cpp
* #### cpp-base64 : https://github.com/ReneNyffenegger/cpp-base64
* #### mem_pool : https://www.codeproject.com/Articles/27487/Why-to-use-memory-pool-and-how-to-implement-it
* #### share_queue : https://stackoverflow.com/questions/36762248/why-is-stdqueue-not-thread-safe
* #### RNN-for-Human-Activity-Recognition-using-2D-Pose-Input : https://github.com/stuarteiffert/RNN-for-Human-Activity-Recognition-using-2D-Pose-Input
* #### LSTM-Human-Activity-Recognition : https://github.com/guillaume-chevalier/LSTM-Human-Activity-Recognition
