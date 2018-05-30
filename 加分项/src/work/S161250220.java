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

    private static final int kernel = 5;

    private static final int latestTaskBeginner = 0; // 其实是一个short
    private static final int lastTaskAmount = latestTaskBeginner + 2;
    private static final int cpuStateBeginner = lastTaskAmount + 2; // CPU个，每个2字节
    private static final int resourceBeginner = cpuStateBeginner + 2 * kernel;
    private static final int pcbTableBeginner = resourceBeginner + Constant.MAX_RESOURCE;
    private static final int pcbFinishBeginner = pcbTableBeginner + 1000 * 2; // 1000个任务，每个任务2
    private static final int pcbBeginner = pcbFinishBeginner + 1000; // 之前用1000个比特

    private static final int PCB_tidBeginner = 0;
    private static final int PCB_arrivedTimeBeginner = 2;
    private static final int PCB_cpuTimeBeginner = 4;
    private static final int PCB_leftTimeBeginner = 6;
    private static final int PCB_rsLengthBeginner = 8;
    private static final int PCB_resourceBeginner = 10;

    // 评估局面好坏的函数，或许不能称他为启发式函数，但是想不到什么好名字了
    private Double heuristic(int taskID) {
        int lastTime = getTaskLastTime(taskID);
        int waitedTime = getTaskWaitTime(taskID);
        Double leastTolerance = priceFunction(lastTime + waitedTime);
        Double delayTolerance = priceFunction(lastTime + waitedTime + getCpuNumber());
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

    private boolean countDownLeft(int taskID){
        int index = getTaskBeginIndex(taskID);
        int leftTime = readShort(index+PCB_leftTimeBeginner);
        if(leftTime == 0) return true;
        leftTime--;
        writeShort(index+PCB_leftTimeBeginner, leftTime);
        if (leftTime == 0) {
            writeFreeMemory(pcbFinishBeginner + taskID, (byte) 1);
            return true;
        }
        return false;
    }

    private boolean isTaskFinish(int taskID){
        return readFreeMemory(pcbFinishBeginner + taskID) == (byte) 1;
    }

    private boolean canUseResource(int taskId) {
        int index = getTaskBeginIndex(taskId);
        int length = readShort(index+PCB_rsLengthBeginner);
        for(int i = 0 ; i < length ; i++){
            byte temp = readFreeMemory(index+PCB_resourceBeginner+i);
            if(readFreeMemory(resourceBeginner+temp-1) != 0) return false;
        }
        return true;
    }

    private void useResource(int taskID){
        int index = getTaskBeginIndex(taskID);
        int length = readShort(index+PCB_rsLengthBeginner);

        for(int i = 0 ; i < length ; i++){
            byte temp = readFreeMemory(index+PCB_resourceBeginner+i);
            writeFreeMemory(resourceBeginner+temp-1, (byte) 1);
        }
    }

    private void clearResource(int taskId) {
        int index = getTaskBeginIndex(taskId);
        int length = readShort(index+PCB_rsLengthBeginner);

        for(int i = 0 ; i < length ; i++) {
            byte temp = readFreeMemory(index + PCB_resourceBeginner + i);
            writeFreeMemory(resourceBeginner + temp - 1, (byte) 0);
        }
    }


    @Override
    public void ProcessSchedule(Task[] arrivedTask, int[] cpuOperate) {

        int taskAmount = readShort(lastTaskAmount);

        if (arrivedTask != null && arrivedTask.length != 0) {
            for (Task task: arrivedTask) {
                recordTask(task);
                taskAmount += 1;
            }
        }

        if (taskAmount == 0)
            return;

        for (int i = 0; i < cpuOperate.length; i++) {
            int taskId = readShort(cpuStateBeginner + 2 * i);
            if (taskId != 0) {
                clearResource(taskId);
            }
        }

        int taskNumber = readShort(latestTaskBeginner);
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

                if (!canUseResource(j)) // 没有足够的资源
                    continue;

                task = j;
                price = current;

            }

            if (task == -1)
                break;
            else {
                cpuOperate[i] = task;
                useResource(task);
                if (countDownLeft(task)) {
                    taskAmount -= 1;
                }
            }
        }


        for (int i = 0; i <= cpuNumber; i++) {
            int preTask = readShort(cpuStateBeginner + 2 * i);
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
                writeShort(cpuStateBeginner + i * 2, cpuOperate[i]);
        }

        writeShort(lastTaskAmount, taskAmount);

    }

    private int getTaskWaitTime(int taskId) {
        return getTimeTick() - readShort(getTaskBeginIndex(taskId) + PCB_arrivedTimeBeginner);
    }

    private int getTaskLastTime(int taskId) {
        return readShort(getTaskBeginIndex(taskId) + PCB_leftTimeBeginner);
    }

    private int getTaskBeginIndex(int taskID){
        return readShort(pcbTableBeginner+taskID*2); // 每个长度是2
    }

    private int getTaskResourceLength(int taskID){
        return readShort(getTaskBeginIndex(taskID)+PCB_rsLengthBeginner);
    }

    private int getNewTaskBeginIndex(){
        int latestTaskID = readShort(latestTaskBeginner);
        if(latestTaskID == 0) return pcbBeginner;
        return getTaskBeginIndex(latestTaskID)+getTaskResourceLength(latestTaskID)+PCB_resourceBeginner;
    }

    private void recordTask(Task task) {
        int newIndex = getNewTaskBeginIndex();
        writeShort(newIndex+PCB_tidBeginner, task.tid);
        writeShort(newIndex+PCB_arrivedTimeBeginner, getTimeTick());
        writeShort(newIndex+PCB_cpuTimeBeginner, task.cpuTime);
        writeShort(newIndex+PCB_leftTimeBeginner, task.cpuTime);
        writeShort(newIndex+PCB_rsLengthBeginner, task.resource.length);
        for(int i = 0 ; i < task.resource.length; i++) {
            writeFreeMemory(newIndex+PCB_resourceBeginner+i, (byte) task.resource[i]);
        }
        writeShort(latestTaskBeginner, task.tid);
        writeShort(pcbTableBeginner+task.tid*2, newIndex);
        writeFreeMemory(pcbFinishBeginner + task.tid, (byte) 0);
    }

    private short readShort(int beginIndex) {
        short ans = 0;
        ans += (readFreeMemory(beginIndex) & 0xff) << 8;
        ans += (readFreeMemory(beginIndex + 1) & 0xff);
        return ans;
    }

    private void writeShort(int beginIndex, int value) {
        writeFreeMemory(beginIndex+1, (byte) ((value&0x000000ff)));
        writeFreeMemory(beginIndex, (byte) ((value&0x0000ff00)>>8));
    }

}