package work;

import bottom.BottomMonitor;
import bottom.BottomService;
import bottom.Constant;
import bottom.Task;
import main.Schedule;

import java.io.IOException;

/**
 *
 * 注意：请将此类名改为 S+你的学号   eg: S161250001
 * 提交时只用提交此类和说明文档
 *
 * 在实现过程中不得声明新的存储空间（不得使用new关键字，反射和java集合类）
 * 所有新声明的类变量必须为final类型
 * 不得创建新的辅助类
 *
 * 可以生成局部变量
 * 可以实现新的私有函数
 *
 * 可用接口说明:
 *
 * 获得当前的时间片
 * int getTimeTick()
 *
 * 获得cpu数目
 * int getCpuNumber()
 *
 * 对自由内存的读操作  offset 为索引偏移量， 返回位置为offset中存储的byte值
 * byte readFreeMemory(int offset)
 *
 * 对自由内存的写操作  offset 为索引偏移量， 将x写入位置为offset的内存中
 * void writeFreeMemory(int offset, byte x)
 *
 * 竟然定义数据结构还要在那片内存上操作，真是太难受了，所以我觉得还是偷懒一些比较好
 * 所以搜索的深度就是1
 * 哈哈哈哈哈
 * 只是为了骗骗分，就不要追求效率的
 *
 */
public class S161250220 extends Schedule {

    private static final int kernel = 2;

    private static final int latestTaskBeginner = 0;
    private static final int cpuStateBeginner = latestTaskBeginner + 4; // CPU个，每个4字节
    private static final int resourceBeginner = cpuStateBeginner + 4 * kernel;
    private static final int pcbTableBeginner = resourceBeginner + Constant.MAX_RESOURCE;
    private static final int pcbBeginner = pcbTableBeginner + 1000 * 4; // 1000个任务，每个任务4

    private static final int PCB_tidBeginner = 0;
    private static final int PCB_arrivedTimeBeginner = 4;
    private static final int PCB_cpuTimeBeginner = 8;
    private static final int PCB_leftTimeBeginner = 12;
    private static final int PCB_rsLengthBeginner = 16;
    private static final int PCB_resourceBeginner = 20;

    // 评估局面好坏的函数，或许不能称他为启发式函数，但是想不到什么好名字了
    private Double heuristic(int taskID) {
        int lastTime = getTaskLastTime(taskID);
        int waitedTime = getTaskWaitTime(taskID);
        Double leastTolerance = priceFunction(lastTime + waitedTime);
        Double delayTolerance = priceFunction(lastTime + waitedTime + 1);
        if (delayTolerance == Double.MAX_VALUE)
            return Double.MAX_VALUE;
        else
            return delayTolerance - leastTolerance;
    }

    private static Double priceFunction(int time) {
        if (time < 5)
            return 1.0;
        else if (time < 15)
            return 2 * time + 1.0;
        else {
            return Math.pow(2, time - 11);
        }
    }

    private void countDownLeft(int taskID){
        int index = getTaskBeginIndex(taskID);
        int leftTime = readInteger(index+PCB_leftTimeBeginner);
        if(leftTime == 0) return;
        leftTime--;
        writeInteger(index+PCB_leftTimeBeginner, leftTime);
    }

    private boolean isTaskFinish(int taskID){
        int index = getTaskBeginIndex(taskID);
        int leftTime = readInteger(index+PCB_leftTimeBeginner);
        return leftTime == 0;
    }

    private void cleanAllResource(){
        for(int i = 0 ; i < Constant.MAX_RESOURCE ; i++){
            writeFreeMemory(resourceBeginner+i, (byte) 0);
        }
    }

    private boolean useResource(int taskID){
        int index = getTaskBeginIndex(taskID);
        int length = readInteger(index+PCB_rsLengthBeginner);

        for(int i = 0 ; i < length ; i++){
            byte temp = readFreeMemory(index+PCB_resourceBeginner+i);
            if(readFreeMemory(resourceBeginner+temp-1) != 0) return false;
        }

        for(int i = 0 ; i < length ; i++){
            byte temp = readFreeMemory(index+PCB_resourceBeginner+i);
            writeFreeMemory(resourceBeginner+temp-1, (byte) 1);
        }

        return true;
    }

    private void clearResource(int taskId) {
        int index = getTaskBeginIndex(taskId);
        int length = readInteger(index+PCB_rsLengthBeginner);

        for(int i = 0 ; i < length ; i++) {
            byte temp = readFreeMemory(index + PCB_resourceBeginner + i);
            writeFreeMemory(resourceBeginner + temp - 1, (byte) 0);
        }
    }


    @Override
    public void ProcessSchedule(Task[] arrivedTask, int[] cpuOperate) {
        if (arrivedTask != null && arrivedTask.length != 0) {
            for (Task task: arrivedTask) {
                recordTask(task);
            }
        }

        cleanAllResource();
        int taskNumber = readInteger(latestTaskBeginner);
        int cpuNumber = getCpuNumber() - 1;
        for (int i = 0; i <= cpuNumber; i++) {
            int task = -1;
            Double price = -1.0;
            for (int j = 1; j <= taskNumber; j++) {
                // 完成的任务直接跳过
                if (isTaskFinish(j))
                    continue;

                // 判断跳过此任务的代价
                Double current = heuristic(j);
                if (current < price)
                    continue;

                if (!useResource(j)) // 没有足够的资源
                    continue;

                task = j;
                price = current;
                clearResource(j); // 释放资源，否则就有可能占着资源不放，导致后来的任务不能公正的和当前任务进行比较

            }

            if (task == -1)
                break;
            else {
                cpuOperate[i] = task;
                useResource(task);
                countDownLeft(task);
            }
        }


        for (int i = 0; i < kernel; i++) {
            int preTask = readInteger(cpuStateBeginner + 4 * i);
            if (preTask == 0) // 遇到了空任务
                continue;
            for(int j = 0; j <= cpuNumber; j++) {
                if (cpuOperate[j] == preTask) {
                    // change pos to reduce the amount of change env
                    int temp = cpuOperate[j];
                    cpuOperate[j] = cpuOperate[i];
                    cpuOperate[i] = temp;
                    break;
                }
            }
        }


        for (int i = 0; i < cpuOperate.length; i++) {
            if (cpuOperate[i] != 0)
                writeInteger(cpuStateBeginner + i * 4, cpuOperate[i]);
        }

    }

    private int getTaskWaitTime(int taskId) {
        return getTimeTick() - readInteger(getTaskBeginIndex(taskId) + PCB_arrivedTimeBeginner);
    }

    private int getTaskLastTime(int taskId) {
        return readInteger(getTaskBeginIndex(taskId) + PCB_leftTimeBeginner);
    }

    private int getTaskBeginIndex(int taskID){
        return readInteger(pcbTableBeginner+taskID*4);
    }

    private int getTaskResourceLength(int taskID){
        return readInteger(getTaskBeginIndex(taskID)+PCB_rsLengthBeginner);
    }

    private int getNewTaskBeginIndex(){
        int latestTaskID = readInteger(latestTaskBeginner);
        if(latestTaskID == 0) return pcbBeginner;
        return getTaskBeginIndex(latestTaskID)+getTaskResourceLength(latestTaskID)+PCB_resourceBeginner;
    }

    private void recordTask(Task task) {
        int newIndex = getNewTaskBeginIndex();
        writeInteger(newIndex+PCB_tidBeginner, task.tid);
        writeInteger(newIndex+PCB_arrivedTimeBeginner, getTimeTick());
        writeInteger(newIndex+PCB_cpuTimeBeginner, task.cpuTime);
        writeInteger(newIndex+PCB_leftTimeBeginner, task.cpuTime);
        writeInteger(newIndex+PCB_rsLengthBeginner, task.resource.length);
        for(int i = 0 ; i < task.resource.length; i++) {
            writeFreeMemory(newIndex+PCB_resourceBeginner+i, (byte) task.resource[i]);
        }
        writeInteger(latestTaskBeginner, task.tid);
        writeInteger(pcbTableBeginner+task.tid*4, newIndex);
    }

    private int readInteger(int beginIndex){
        int ans = 0;
        ans += (readFreeMemory(beginIndex)&0xff)<<24;
        ans += (readFreeMemory(beginIndex+1)&0xff)<<16;
        ans += (readFreeMemory(beginIndex+2)&0xff)<<8;
        ans += (readFreeMemory(beginIndex+3)&0xff);
        return ans;
    }

    private void writeInteger(int beginIndex, int value){
        writeFreeMemory(beginIndex+3, (byte) ((value&0x000000ff)));
        writeFreeMemory(beginIndex+2, (byte) ((value&0x0000ff00)>>8));
        writeFreeMemory(beginIndex+1, (byte) ((value&0x00ff0000)>>16));
        writeFreeMemory(beginIndex, (byte) ((value&0xff000000)>>24));
    }

    /**
     * 执行主函数 用于debug
     * 里面的内容可随意修改
     * 你可以在这里进行对自己的策略进行测试，如果不喜欢这种测试方式，可以直接删除main函数
     * @param args 没什么乱用的参数
     * @throws IOException 实际上不会抛出的异常
     */
    public static void main(String[] args) throws IOException {
        // 定义cpu的数量
        int cpuNumber = 2;
        // 定义测试文件
        String filename = "src/testFile/textSample.txt";

        BottomMonitor bottomMonitor = new BottomMonitor(filename,cpuNumber);
        BottomService bottomService = new BottomService(bottomMonitor);
        Schedule schedule =  new S161250220();
        schedule.setBottomService(bottomService);

        //外部调用实现类
        for(int i = 0 ; i < 500 ; i++){
            Task[] tasks = bottomMonitor.getTaskArrived();
            int[] cpuOperate = new int[cpuNumber];

            // 结果返回给cpuOperate
            schedule.ProcessSchedule(tasks,cpuOperate);

            try {
                bottomService.runCpu(cpuOperate);
            } catch (Exception e) {
                System.out.println("Fail: "+e.getMessage());
                e.printStackTrace();
                return;
            }
            bottomMonitor.increment();
        }

        //打印统计结果
        bottomMonitor.printStatistics();
        System.out.println();

        //打印任务队列
        bottomMonitor.printTaskArrayLog();
        System.out.println();

        //打印cpu日志
        bottomMonitor.printCpuLog();


        if(!bottomMonitor.isAllTaskFinish()){
            System.out.println(" Fail: At least one task has not been completed! ");
        }
    }

}
