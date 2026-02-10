#include "async_task_manager.hpp"
#include <stdexcept>

AsyncTaskManager::AsyncTaskManager() : streamingRunning(true) {
    try {
        deepSeekClient = make_unique<DeepSeekClient>();
        streamingThread = thread(&AsyncTaskManager::streamingWorkerFunction, this);
    } catch (const exception& e) {
        streamingRunning = false;
        throw;
    }
}

AsyncTaskManager::~AsyncTaskManager() {
    streamingRunning = false;
    streamingCondition.notify_all();
    if (streamingThread.joinable()) {
        streamingThread.join();
    }
}

void AsyncTaskManager::setOnCommandResultCallback(function<void(const json&)> callback) {
    if (deepSeekClient) {
        deepSeekClient->setOnCommandResultCallback(callback);
    }
}

void AsyncTaskManager::streamingWorkerFunction() {
    while (streamingRunning) {
        StreamingTask task;
        
        {
            unique_lock<mutex> lock(streamingMutex);
            streamingCondition.wait(lock, [this]() { 
                return !streamingQueue.empty() || !streamingRunning; 
            });
            
            if (!streamingRunning) break;
            
            task = streamingQueue.front();
            streamingQueue.pop();
        }
        
        // Verificar cancel·lació abans d'executar
        if (task.cancelFlag && task.cancelFlag->load()) {
            delete task.cancelFlag;
            continue;
        }
        
        deepSeekClient->chatStream(task.message, task.onChunk, task.useTools);
        
        // Netejar el flag de cancel·lació
        if (task.cancelFlag) {
            delete task.cancelFlag;
        }
        
        if (task.onComplete) {
            task.onComplete();
        }
    }
}

void AsyncTaskManager::cancelCurrentTask() {
    if (deepSeekClient) {
        deepSeekClient->cancelCurrentRequest();
    }
    
    // També podem netejar la cua si volem cancel·lar tot
    {
        lock_guard<mutex> lock(streamingMutex);
        while (!streamingQueue.empty()) {
            auto& task = streamingQueue.front();
            if (task.cancelFlag) {
                task.cancelFlag->store(true);
            }
            streamingQueue.pop();
        }
    }
}

bool AsyncTaskManager::isTaskCancellable() const {
    return deepSeekClient && deepSeekClient->isCancelRequested();
}

void AsyncTaskManager::submitStreamingTask(const string& message, 
                                          function<void(const string&)> onChunk,
                                          function<void()> onComplete,
                                          bool useTools) {
    StreamingTask task;
    task.message = message;
    task.useTools = useTools;
    task.onChunk = onChunk;
    task.onComplete = onComplete;
    task.cancelFlag = new atomic<bool>(false);

    {
        lock_guard<mutex> lock(streamingMutex);
        streamingQueue.push(move(task));
    }
    
    streamingCondition.notify_one();
}

void AsyncTaskManager::clearChatHistory() {
    if (deepSeekClient) {
        deepSeekClient->clearChatHistory();
    }
}

bool AsyncTaskManager::changeDirectory(const string& dir) {
    if (deepSeekClient) {
        return deepSeekClient->changeDirectory(dir);
    }
    return false;
}

string AsyncTaskManager::getCurrentDirectory() const {
    if (deepSeekClient) {
        return deepSeekClient->getCurrentDirectory();
    }
    return "";
}

bool AsyncTaskManager::isInitialized() const {
    return deepSeekClient != nullptr;
}

vector<json> AsyncTaskManager::getLastCommandResults() const {
    if (deepSeekClient) {
        return deepSeekClient->getLastCommandResults();
    }
    return {};
}

vector<json> AsyncTaskManager::getAndClearPendingCommandResults() {
    if (deepSeekClient) {
        return deepSeekClient->getAndClearPendingCommandResults();
    }
    return {};
}