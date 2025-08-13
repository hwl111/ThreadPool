# C++ ThreadPool 线程池项目

一个高性能的C++线程池实现，支持固定线程数和动态线程数两种模式，提供灵活的任务提交接口和返回值获取功能。

## 项目特性

### 🚀 核心功能
- **双模式支持**: 固定线程数模式(FIXED)和动态线程数模式(CACHED)
- **任务队列管理**: 支持任务队列大小限制和超时处理
- **返回值获取**: 支持获取异步任务的执行结果
- **线程生命周期管理**: 自动管理线程的创建、回收和销毁
- **线程安全**: 使用互斥锁和条件变量保证线程安全

### 📁 项目结构
```
ThreadPool/
├── ThreadPool/                    # 基础版本
│   ├── threadpool.h              # 线程池头文件
│   ├── threadpool.cpp            # 线程池实现
│   ├── main.cpp                  # 使用示例
│   └── ThreadPool.vcxproj        # Visual Studio项目文件
├── ThreadPool-final-version/     # 最终版本(推荐使用)
│   ├── threadpool.h              # 改进版线程池头文件
│   ├── main.cpp                  # 改进版使用示例
│   └── ThreadPool-final-version.vcxproj
└── README.md                     # 项目说明文档
```

## 快速开始

### 编译环境
- **编译器**: Visual Studio 2019/2022 (支持C++11及以上)
- **标准**: C++11/14/17
- **平台**: Windows (可移植到Linux)

### 编译运行
1. 使用Visual Studio打开 `ThreadPool.sln`
2. 选择 `ThreadPool-final-version` 项目
3. 编译并运行

## 使用示例

### 基础用法

```cpp
#include "threadpool.h"

int main() {
    // 创建线程池
    ThreadPool pool;
    
    // 设置模式为动态线程数
    pool.setMode(PoolMode::MODE_CACHED);
    
    // 启动线程池，初始线程数为1
    pool.start(1);
    
    // 提交任务并获取结果
    std::future<int> result = pool.submitTask([](int a, int b) {
        return a + b;
    }, 10, 20);
    
    // 获取任务结果
    int sum = result.get();
    std::cout << "Result: " << sum << std::endl;
    
    return 0;
}
```

### 复杂任务示例

```cpp
#include "threadpool.h"

// 自定义任务类
class MyTask : public Task {
public:
    MyTask(int begin, int end) : begin_(begin), end_(end) {}
    
    Any run() override {
        std::cout << "Thread " << std::this_thread::get_id() << " processing..." << std::endl;
        
        unsigned long long sum = 0;
        for (unsigned long long i = begin_; i <= end_; i++) {
            sum += i;
        }
        
        return sum;
    }
    
private:
    int begin_;
    int end_;
};

int main() {
    ThreadPool pool;
    pool.setMode(PoolMode::MODE_CACHED);
    pool.start(2);
    
    // 提交多个任务
    Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 10000000));
    Result res2 = pool.submitTask(std::make_shared<MyTask>(10000001, 20000000));
    Result res3 = pool.submitTask(std::make_shared<MyTask>(20000001, 30000000));
    
    // 获取结果
    unsigned long long sum1 = res1.get().cast_<unsigned long long>();
    unsigned long long sum2 = res2.get().cast_<unsigned long long>();
    unsigned long long sum3 = res3.get().cast_<unsigned long long>();
    
    std::cout << "Total sum: " << (sum1 + sum2 + sum3) << std::endl;
    
    return 0;
}
```

## API 参考

### ThreadPool 类

#### 构造函数
```cpp
ThreadPool();  // 默认构造函数
```

#### 主要方法

##### 启动线程池
```cpp
void start(int initThreadSize = std::thread::hardware_concurrency());
```
- `initThreadSize`: 初始线程数，默认为CPU核心数

##### 设置线程池模式
```cpp
void setMode(PoolMode mode);
```
- `mode`: 线程池模式
  - `PoolMode::MODE_FIXED`: 固定线程数模式
  - `PoolMode::MODE_CACHED`: 动态线程数模式

##### 提交任务 (最终版本)
```cpp
template<typename Func, typename... Args>
auto submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>;
```
- 支持任意函数和参数
- 返回 `std::future` 对象用于获取结果

##### 提交任务 (基础版本)
```cpp
Result submitTask(std::shared_ptr<Task> sp);
```
- 需要继承 `Task` 类并实现 `run()` 方法
- 返回 `Result` 对象用于获取结果

##### 配置参数
```cpp
void setTaskQueMaxThreshHold(int threshhold);     // 设置任务队列最大长度
void setThreadSizeThreshHold(int threshhold);     // 设置最大线程数(CACHED模式)
```

## 设计模式

### 线程池模式

#### FIXED 模式
- 线程数量固定，不会动态增减
- 适合任务量稳定的场景
- 资源占用可预测

#### CACHED 模式
- 线程数量根据任务量动态调整
- 空闲线程会在指定时间后自动回收
- 适合任务量波动较大的场景

### 核心组件

#### Any 类
- 类型擦除容器，可以存储任意类型的数据
- 支持类型安全的类型转换
- 用于任务返回值的统一处理

#### Result 类
- 封装任务执行结果
- 提供同步等待机制
- 支持获取 `Any` 类型的返回值

#### Semaphore 类
- 信号量实现
- 用于线程间同步
- 支持资源计数和等待机制

## 性能特性

### 线程管理
- **自动负载均衡**: 任务自动分配给空闲线程
- **动态扩缩容**: CACHED模式下根据负载自动调整线程数
- **资源回收**: 空闲线程自动回收，避免资源浪费

### 任务调度
- **FIFO队列**: 任务按提交顺序执行
- **超时处理**: 任务提交支持超时机制
- **异常安全**: 任务执行异常不会影响线程池稳定性

### 内存管理
- **智能指针**: 使用 `std::unique_ptr` 和 `std::shared_ptr` 管理资源
- **RAII**: 自动资源管理，避免内存泄漏
- **移动语义**: 支持高效的资源转移

## 注意事项

1. **线程安全**: 所有公共接口都是线程安全的
2. **生命周期**: 线程池析构时会等待所有任务完成
3. **异常处理**: 任务执行异常会被捕获，不会影响其他任务
4. **资源限制**: 建议根据系统资源合理设置线程数和队列大小

## 版本说明

### ThreadPool-final-version (推荐)
- 使用 `std::future` 和 `std::packaged_task`
- 支持可变参数模板
- 更简洁的API接口
- 更好的类型安全

### ThreadPool (基础版本)
- 使用自定义的 `Result` 和 `Any` 类
- 需要继承 `Task` 类
- 更详细的实现展示

## 贡献

欢迎提交Issue和Pull Request来改进这个项目！

## 许可证

本项目采用 MIT 许可证。 