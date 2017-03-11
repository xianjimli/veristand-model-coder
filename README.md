# Veristand-model-coder (Veristand C/C++模型框架代码产生器)

[NI Veristand](http://www.ni.com/veristand/)是一个强大的『实时系统测试软件』的开发环境，你可以用它开发各种实时系统的测试软件。其中一个非常有意思的东西就是，它可以用软件开发的模型来代替真实的被测对象，在开发过程中，不需要实际的设备也可以运行测试软件，大大提高了软件开发的效率。模型可以用Matlab的simulink、LabView和C/C++来开发。

模型到底是什么呢？从不同的方面看，答案是不一样的：

* 从概念上看，模型是用数学方程对真实事物的抽象和模拟。比如一个电机模型就是对真实电机模拟，它具有电机的特性：给定一个电压可以得到一定的转速，同时它又是抽象的，我们只关心需要的参数，而忽略不重要的参数，从而简化模型的实现。

* 从形式上看，模型就是一个DLL，它对外提供一组函数。你可以用任何编程语言来开发一个模型，也可以用Matlab Simulink或LabView来建立一个模型，然后把它编译成DLL。

* 从程序来看，模型就是一个函数，函数有输入数据和输出结果。除了输入数据外，参数和时间也会影响输出结果。

在NI Vesistand中：

* 输入数据称为Inports，对于一个电机模型来说，电机控制器发过来的命令就是Inports。

* 输出结果称为Outports，对于一个电机模型来说，电机实际的转速就是Outports。

* 参数称为Parameters，参数一般是指在初始化时设置，而在整个模拟过程中不经常改变的变量。对于一个电机模型来说，负载可以看作是参数。

* 还有一种输出数据称为Signals，Signals可以看作是在模型内部设置的探针或测试点，用来观察模型的内部状态，比如在电机模型内部设置一个温度探测器，这样就可以实时观察到电机的温度。


在NI Vesistand中，负责模型计算的函数是NIRT_Schedule。但是除此之外，模型还需要向NI Vesistand提供一堆描述信息，用C/C++来写一个模型，哪怕是一个简单的输出等于输入的模型，也要写上几百行代码。veristand-model-coder是一个用来简化C/C++开发模型的小工具，让你只需要关注模型本身的方程即可。


veristand-model-coder需要一个描述模型输入、输出、参数和Signal的JSON文件和一个实现模型计算的函数，veristand-model-coder根据它们生成模型的框架代码和Makefile。

### 使用方法

下面我们以一个例子介绍veristand-model-coder的使用方法：

0.安装node.js和cmake，并下载veristand-model-coder，进入veristand-model-coder目录。

1.编写JSON文件(这里用现存的demos/sine-definition.json)。

2.编写模型的计算函数(这里用现存的demos/sine-impl.c)。

3.生成模型相关文件。这里模型的名称是sinewave，生成的代码在sinewave目录下。

```
node coder.js demos/sine-definition.json
```

2.用CMake生成VS Studio的工程文件(也可以手工建立一个DLL工程)。

```
cd sinewave
cmake ..
```

4.用VS Studio编译生成sinewave.dll。

5.在NI Veristand中加载模型sinewave.dll。

### 模型描述文件

描述文件包括几个部分：

* 模型本身的信息。包括名称、描述、baserate和模型实现文件名(ImplFileName)。
* 参数描述信息(Parameters)。包括名称、类型、描述和缺省值。
* 输入描述信息(Inports)。包括名称、类型和描述。
* 输出描述信息(Outports)。包括名称、类型和描述。
* 信号描述信息(Signals)。包括名称、类型和描述。


### 模型实现文件

在模型实现文件中，定义自己的模型计算函数，如下面这个模型计算函数，把输入的数据乘以指定的参数gain作为输出的结果：

```
/* INPUT: *inData, pointer to inport data at the current timestamp, to be 
  	      consumed by the function
   OUTPUT: *outData, pointer to outport data at current time + baserate, to be
  	       produced by the function
   INPUT: timestamp, current simulation time */
int32_t USER_TakeOneStep(double *inData, double *outData, double timestamp) 
{
	if (inData) rtInport.In1 = inData[0];

	rtSignal.gain = readParam.gain;
	rtOutport.Out1 = rtSignal.gain * rtInport.In1;			
	
	if (outData) outData[0] = rtOutport.Out1;	
	
	return NI_OK;
}
```














