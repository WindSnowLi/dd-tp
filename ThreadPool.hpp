#pragma once

#include <thread>
#include <memory>
#include <functional>
#include <utility>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <vector>
#include <future>

class ThreadAdapter {
protected:
    bool stop = false;

    std::shared_ptr<std::thread> m_t;

    /**
    * @brief 子线程阻塞锁
    *
    */
    std::mutex m_mutex;
public:
    enum class ThreadStatus {
        WAIT,
        RUNNING,
    };
    std::atomic<ThreadStatus> status = ThreadStatus::WAIT;

    explicit ThreadAdapter(std::queue<std::function<void()>> &fq, std::condition_variable &cv, std::mutex &fqM) {
        m_t = std::make_shared<std::thread>([this, &q = fq, &c = cv, &m = fqM]() {
            while (!stop) {
                std::function<void()> fun;
                while (true) {
                    {
                        std::unique_lock<std::mutex> l(m);
                        if (!q.empty()) {
                            fun = q.front();
                            q.pop();
                            break;
                        }
                    }
                    std::unique_lock<std::mutex> lock(m_mutex);
                    c.wait(lock);
                }
                this->status = ThreadStatus::RUNNING;
                if (fun) {
                    fun();
                }
                this->status = ThreadStatus::WAIT;
                c.notify_all();
            }
        });
    }

    void Detach() {
        if (this->status != ThreadStatus::WAIT && m_t->joinable()) {
            this->stop = true;
            m_t->detach();
        }
    }

    ~ThreadAdapter() {
        if (this->status != ThreadStatus::WAIT && m_t->joinable()) {
            this->stop = true;
            m_t->join();
        }
    }
};


class ThreadPool {
protected:
    std::queue<std::function<void()>> fq;

    std::mutex fqM;

    std::condition_variable cv;

    std::vector<std::shared_ptr<ThreadAdapter>> t;
public:
    explicit ThreadPool(int s) {
        for (int i = 0; i < s; ++i) {
            auto p = std::make_shared<ThreadAdapter>(fq, cv, fqM);
            t.emplace_back(std::move(p));
        }
    }

    /**
 * @brief 主要核心思想就这一个模板函数，用于将原始函数封装成
 *
 * @tparam F 目标函数类型
 * @tparam Args 函数的参数类型
 * @param func 目标函数
 * @param args 目标参数
 * @return decltype(auto) 自动推导返回值
 */
    template<typename F, typename... Args>
    decltype(auto) AddTask(F &&func, Args &&...args) {
        //推导最后的返回值类型
        using res_type = typename std::result_of<F(Args...)>::type;
        //封装为packaged_task的智能指针类型
        auto task = std::make_shared<std::packaged_task<res_type()>>(
                std::bind(std::forward<F>(func), std::forward<Args>(args)...));
        //获取用于异步获取返回值的future
        std::future<res_type> res = task->get_future();
        {
            {
                std::unique_lock<std::mutex> lock(fqM);
                fq.emplace([task]() { (*task)(); });
            }
            cv.notify_one();
        }
        return res;
    }
};