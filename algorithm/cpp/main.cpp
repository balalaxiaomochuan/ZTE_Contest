#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <fstream>
#include <queue>
#include <deque>
#include <list>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
using namespace std;

class port;
class flow;
// 为流选择合适的端口
int selectSmallestPort(const vector<port>& ports, int bandwith);
// 判断是否所有端口等待区为空
bool isAllPortsWaitFull(const vector<port>& , int);
vector<int> splitStrToInts(char *str, int sz);
vector<flow> getFlowsOfTime(int );
int getDirNum(const string dirPath);


// 流类
class flow{
public:
    flow(int id, int bandwith, int entertime, int timetosend) :
            fid(id), fBandwith(bandwith), EnterTime(entertime), TimeToSend(timetosend) {}

    bool operator<(const flow& rhs) const {
        if(rhs.EnterTime == EnterTime){
            if(TimeToSend == rhs.TimeToSend)
                return fBandwith < rhs.fBandwith;
            return TimeToSend < rhs.TimeToSend;
        }
        return EnterTime < rhs.EnterTime;
    }


private:
    int fid = 0;
    int fBandwith;
    int EnterTime;
    int TimeToSend;
    int TimeOfSendEnd = -1;               // 从端口发送完的时间
    int selectPort = -1;             // 选择的发送端口
    int TimeOfStartTosend = -1;       // 开始在端口发送的时间

public:
    inline int getFid() const { return fid;}

    inline int getFBandwith() const {return fBandwith;}

    inline int getEnterTime() const {return EnterTime;}

    inline int getSelectPort() const {return selectPort;}

    inline int getEndingTime() const {return TimeOfStartTosend + TimeToSend; }

    inline int getTimeOfStartTosend() const {return TimeOfStartTosend;}

    inline int getTimeToSend() const {return TimeToSend;}

    inline void changTimeOfStartTosend(int time) {TimeOfStartTosend = time;}

    inline void changSelectPort(int p) {selectPort = p;}


};

// 用于给堆的排序
struct cmp{
    bool operator()(const flow& a, const flow& b){
        return a.getEndingTime() > b.getEndingTime();
    }
};

// 端口类
class port{
public:
    // 构造函数
    port(int id, int bandwith) :
            pid(id), pBandWith(bandwith), leavBandwith(bandwith) {}


    bool operator<(const port& rhs) const {
        return leavBandwith < rhs.leavBandwith;
    }

private:
    int pid;
    int pBandWith;
    int leavBandwith;                 // 端口的剩余带宽
    int PRIORITY;                 // 流进入端口的优先级，缓冲区所有流*发送耗时 / 端口总带宽

public:
    deque<flow> waitBuf;      // 端口等待区的流
    // list<flow> sendingFlow;  // 正在发送的流
    priority_queue<flow, vector<flow>, cmp> sendingFlow;

public:
    // 获取端口信息
    void printInfo() {
        cout << "当前端口信息： id :" << getPid() << ", 排队区长度： "<<  waitBuf.size() << "，传输区长度 ： " << sendingFlow.size()
             << ",当前带宽: " << leavBandwith << ", 总带宽： " << pBandWith << endl;
    }

    inline int getPid() const { return pid;}

    inline int getPBandwith( ) const {return pBandWith;}

    inline int getleavBandwith() const {return leavBandwith;}

    inline bool isWaitBufEmpty() const {return waitBuf.size() == 0 ? true : false; }

    inline int getSendingFlowSZ() const {return sendingFlow.size();}

    inline int getWaitBufSZ() const {return waitBuf.size();}

    inline void pushSendingFlow(flow sendflow) {
        sendingFlow.push(sendflow);
        leavBandwith -= sendflow.getFBandwith();
        PRIORITY +=  sendflow.getTimeToSend() * sendflow.getFBandwith();
    }

    inline void popSendingFlow() {
        flow sendflow = sendingFlow.top();
        sendingFlow.pop();
        leavBandwith += sendflow.getFBandwith();
        PRIORITY -=  sendflow.getTimeToSend() * sendflow.getFBandwith();
    }

    inline void pushWaitBuf(flow waitflow) {
        waitflow.changSelectPort(pid);
        waitBuf.push_back(waitflow);
        PRIORITY +=  waitflow.getTimeToSend() * waitflow.getFBandwith();
    }

    inline void popWaitBuf() {
        flow waitflow = waitBuf.front();
        waitBuf.pop_front();
        PRIORITY -=  waitflow.getTimeToSend() * waitflow.getFBandwith();
    }

    inline double getPriority(){
        return (double)PRIORITY / pBandWith;
    }

};



bool isAllPortsWaitFull(const vector<port>& ports, int bandwith){
    for(auto p : ports){
        if(p.isWaitBufEmpty() && p.getleavBandwith() >= bandwith) return false;
    }
    return true;
}

int selectSmallestPort(const vector<port>& ports, int bandwith){
    int minBW = INT_MAX;
    int ret = -1;
    for(int i=0; i<ports.size(); i++){
        if(ports[i].getleavBandwith() >= bandwith && minBW > ports[i].getleavBandwith() ){
            minBW = ports[i].getleavBandwith();
            ret = i;
            // cout << "选择最小端口中, i = " << i << endl;
        }
    }
    return ret;
}

vector<int> splitStrToInts(char *str, int sz){
    vector<int> ret;
    string tmp;
    for(int i=0; i<sz; i++){
        if(str[i] != ','){
            tmp.push_back(str[i]);
        }
        else{
            ret.push_back(atoi(tmp.c_str()));
            tmp = "";
        }
    }
    ret.push_back(atoi(tmp.c_str()));
    return ret;
}


// 获取指定路径下文件夹的数目
int getDirNum(const string path){
    // int ret = 0;
    DIR* dirp = opendir(path.c_str());
    struct dirent* dp;
    int folder_count = 0;

    while ((dp = readdir(dirp)) != NULL) {
        if (dp->d_type == DT_DIR &&
            std::string(dp->d_name) != "." &&
            std::string(dp->d_name) != "..") {
            ++folder_count;
        }
    }
    return folder_count;
}

bool AllPortsFinished(const vector<port> &ports){
    for(auto port : ports){
        if(port.getSendingFlowSZ()) return false;
    }
    return true;
}


// 贪心+模拟大法
int main(){
    string datadir = "data";
    int dirnum = getDirNum("../"+datadir);
    cout << dirnum << endl;
    for(int DirIdx = 0; DirIdx < dirnum; DirIdx++){
        // 读取port文件
        ifstream PortInFile;
        cout << "process : " << to_string(DirIdx) << endl;
        string portpath = "../"+datadir+"/" + to_string(DirIdx) + "/port.txt";
        PortInFile.open(portpath.c_str(), ios::in);

        if(!PortInFile.is_open()){
            cout << DirIdx << "port.txt打开失败!" << endl;
            exit(1);
        }

        // 用向量保存端口信息
        vector<port> ports;

        char buf[1024] = {0};
        while(PortInFile >> buf){
            if(!(buf[0] >= '0' && buf[0] <= '9')) continue;
            vector<int> tmp = splitStrToInts(buf, strlen(buf));

            // tmp[0] = id, tmp[1] = 带宽
            ports.push_back(port(tmp[0], tmp[1]));

        }

        // 读取flow文件
        ifstream FlowInFile;
        string flowpath = "../"+datadir+"/" + to_string(DirIdx) + "/flow.txt";
        FlowInFile.open(flowpath.c_str(), ios::in);

        if(!FlowInFile.is_open()){
            cout << "flow.txt打开失败!" << endl;
            exit(1);
        }

        // 用向量保存流信息
        deque<flow> flows;

        buf[1024] = {0};
        while(FlowInFile >> buf){
            if(!(buf[0] >= '0' && buf[0] <= '9')) continue;
            vector<int> tmp = splitStrToInts(buf, strlen(buf));
            // tmp[0] = id, tmp[1] = 带宽, tmp[2] = 起始时间， tmp[3] = 发送时间
            flows.push_back(flow(tmp[0], tmp[1], tmp[2], tmp[3]));

        }

        PortInFile.close();
        FlowInFile.close();


        string resultpath = "../"+datadir+"/" + to_string(DirIdx) + "/result.txt";
        ofstream fout(resultpath);
        if(!fout){
            cout << "result.txt文件创建失败" << endl;
            exit(0);
        }

        sort(flows.begin(), flows.end());

        int time = 0;
        int AAA = 0;
        int flowLen = flows.size();
        while(!(time != 0 && AllPortsFinished(ports))){
            // 先判断当前时间time有无流传输完毕
            for(int p = 0; p < ports.size(); p++){

                if(ports[p].getSendingFlowSZ() == 0) continue;
                while(ports[p].getSendingFlowSZ() > 0 && ports[p].sendingFlow.top().getEndingTime() == time){

                    ports[p].popSendingFlow();
                }

                // 查看当前是否有足够带宽给排队区流占用
                while(ports[p].getWaitBufSZ() > 0 && ports[p].waitBuf.front().getFBandwith() <= ports[p].getleavBandwith()){
                    ports[p].waitBuf.front().changTimeOfStartTosend(time);
                    auto pf = ports[p].waitBuf.front();
                    fout << pf.getFid() << "," << pf.getSelectPort()
                         << "," << pf.getTimeOfStartTosend() << endl;
                    ports[p].pushSendingFlow(ports[p].waitBuf.front());
                    ports[p].popWaitBuf();
                }

            }

            // cout << "调试" << endl;
            // 优先填充所有有剩余带宽的端口
            while(flows.size() > 0 && time >= flows.front().getEnterTime() && !isAllPortsWaitFull(ports, flows.front().getFBandwith())){
                int spid = selectSmallestPort(ports, flows.front().getFBandwith());
                flows.front().changTimeOfStartTosend(time);
                flows.front().changSelectPort(ports[spid].getPid());

                fout << flows.front().getFid() << "," << flows.front().getSelectPort()
                     << "," << flows.front().getTimeOfStartTosend() << endl;

                // 此处应写入进入时间
                ports[spid].pushSendingFlow(flows.front());
                flows.pop_front();

            }

            // 下一次流传输完成的时间节点
            int nextTime = INT_MAX;
            for(int p = 0; p < ports.size(); p++){
                if(ports[p].getSendingFlowSZ() == 0) continue;
                auto tpFlow = ports[p].sendingFlow.top();

                if(tpFlow.getEndingTime() < nextTime){
                    nextTime = tpFlow.getEndingTime();
                }
            }

            // cout << "处理等待区" << endl;
            // 到了此处说明只能填充等待区,先选择优先级最高的端口(得到索引)
            double priority = INT_MAX;
            int selPortIdx;
            for(int p = 0; p < ports.size(); p++){
                // 寻找优先级最高（数值最小）的端口进行插入，前提是该端口带宽需要大于流带宽
                if(flows.front().getFBandwith() > ports[p].getPBandwith()) continue;

                if(ports[p].getPriority() < priority){
                    // cout << "selPortIdx" << selPortIdx << endl;
                    priority = ports[p].getPriority();
                    selPortIdx = p;
                }

            }


            if(flows.size() > 0 && flows.front().getEnterTime() <= time){
                // cout << "selPortIdx  " << selPortIdx << endl;

                ports[selPortIdx].pushWaitBuf(flows.front());
                flows.pop_front();
            }
            // cout << "结束区" << endl;

            time = time+1 < nextTime ? time+1 : nextTime;


        }



        fout.close();
    }


    return 0;
}
