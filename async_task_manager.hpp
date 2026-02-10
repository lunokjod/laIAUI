#ifndef ASYNC_TASK_MANAGER_HPP
#define ASYNC_TASK_MANAGER_HPP

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <atomic>
#include <nlohmann/json.hpp>
#include "deepseek_client.hpp"

using json = nlohmann::json;
using namespace std;

class AsyncTaskManager {
private:
    struct StreamingTask {
        string message;
        bool useTools;
        function<void(const string&)> onChunk;
        function<void()> onComplete;
        atomic<bool>* cancelFlag;
    };
    
    queue<StreamingTask> streamingQueue;
    mutex streamingMutex;
    condition_variable streamingCondition;
    thread streamingThread;
    atomic<bool> streamingRunning;
    unique_ptr<DeepSeekClient> deepSeekClient;
    
    void streamingWorkerFunction();
    
public:
    AsyncTaskManager();
    ~AsyncTaskManager();
    
    void setOnCommandResultCallback(function<void(const json&)> callback);
    void submitStreamingTask(const string& message, 
                            function<void(const string&)> onChunk,
                            function<void()> onComplete,
                            bool useTools = true);
    
    void clearChatHistory();
    bool changeDirectory(const string& dir);
    string getCurrentDirectory() const;
    bool isInitialized() const;
    vector<json> getLastCommandResults() const;
    vector<json> getAndClearPendingCommandResults();
    void cancelCurrentTask();
    bool isTaskCancellable() const;

};

#endif