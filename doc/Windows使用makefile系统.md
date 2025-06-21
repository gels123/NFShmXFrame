### win安装make
    pkg目录下有一个win_make.zip，解压放到C盘，把win_make的bin目录C:\win_make\chocolatey\bin，放到Windows的环境变量path里。Path=...;C:\win_make\chocolatey\bin
### win安装python2.7
    安装python2.7, pkg目录下有python-2.7xxx.msi, 安装好，把python27的bin目录C:\Python27，加到windows环境变量path里。Path=....;C:\Python27;
### win安装protobuf-3.6.0
    pkg目录下解压protobuf-3.6.0.zip， 把tools目录下protoc.exe复制到protobuf-3.6.0的src目录下。
### 安装git,一般自带git.bash, 点击邮件就有Open Git Bash Here, 安装protobuf的python部分
    打开git.bash, cd到protobuf-3.6.0/python目录下，输入命令python setup.py build;python setup.py test;python setup.py install;
### clion有更好的方案
    当然如果你使用clion的话，在clion的设置里找到“终端”，设置终端shell路径为git.bash，比如:C:\Program Files\Git\bin\bash.exe;然后设置终端的目录，
    终端的环境变量path里加好make的目录C:\win_make\chocolatey\bin和python的目录C:\Python27;
    这样的话，你就可以在clion直接使用终端，然后make.
### 在bash里，cd到makefiles，比如game/TestGame/makefiles, 直接输入命令make， 就可以使用makefile系统，自动解析Excel文件，生成proto,代码，以及bin文件

    根据你提供的文档内容，我将其整理为更清晰的步骤以便于理解和操作。以下是Windows环境下使用Makefile系统的安装和配置指南：

### Windows下使用Makefile系统安装指南

#### 1. 安装Make工具
- 解压`pkg`目录下的`win_make.zip`至C盘。
- 将解压后的`win_make\chocolatey\bin`路径（即`C:\win_make\chocolatey\bin`）添加到系统的环境变量`Path`中。

#### 2. 安装Python 2.7
- 使用`pkg`目录下的`python-2.7xxx.msi`文件进行Python 2.7的安装。
- 安装完成后，将Python的安装路径（默认为`C:\Python27`）添加到系统的环境变量`Path`中。

#### 3. 安装Protobuf 3.6.0
- 解压`pkg`目录下的`protobuf-3.6.0.zip`。
- 将解压后文件夹中的`protoc.exe`从`tools`目录复制到`protobuf-3.6.0\src`目录下。
- 打开Git Bash（如果已安装），切换到`protobuf-3.6.0/python`目录，并依次执行以下命令以完成Python部分的安装：
  ```bash
  python setup.py build
  python setup.py test
  python setup.py install
  ```


#### 4. CLion集成配置（可选）
如果你使用的是CLion开发环境，可以通过以下方式简化命令行工具的使用：
- 在CLion设置中找到“终端”选项。
- 设置终端Shell路径为Git Bash的路径，例如：`C:\Program Files\Git\bin\bash.exe`。
- 在终端的环境变量`Path`中加入Make工具路径`C:\win_make\chocolatey\bin`以及Python路径`C:\Python27`。
- 完成上述配置后，即可在CLion内置的终端中直接运行`make`命令。

#### 5. 使用Makefile系统
- 打开Git Bash或CLion内置终端，切换到包含Makefile的项目目录（如`game\TestGame\makefiles`）。
- 输入`make`命令，Makefile系统将会自动解析Excel文件，生成相应的Proto、代码及Bin文件。

以上就是整个安装过程的详细说明，希望对你有所帮助！如果有任何疑问或者遇到问题，请随时提问。