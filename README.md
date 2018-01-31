UVCCamera　/ OpenCVwithUVC
=========

Sample app to show how to apply image processing to the images
that came from UVC web camera by OpenCV on non-rooted Android device.
This app is based on `usbWebCameraTest8` on `UVCCamera` repository
https://github.com/saki4510t/UVCCamera

Copyright (c) 2014-2018 saki t_saki@serenegiant.com

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

All files in the folder are under this Apache License, Version 2.0.
Files in the jni/opencv3 folders have a different license,
see the respective files.


## How to compile(not ready now)

Note: Just make sure that `local.properties` contains the paths for `sdk.dir` and `ndk.dir`. Or you can set them as enviroment variables of you shell. On some system, you may need add `JAVA_HOME` envairoment valiable that points to JDK directory.  

```
sdk.dir={path to Android SDK on your storage}
ndk.dir={path to Android SDK on your storage}
```
