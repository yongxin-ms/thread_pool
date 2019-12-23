#include <stdio.h>
#include <string>
#include <list>
#include "thread_pool.h"

class CalPoolExample {
public:
	CalPoolExample()
		: pool_(3){};
	~CalPoolExample() { pool_.ShutDown(); }

	void Init() { pool_.Init(); }

	int Update() {
		int count = 0;
		for (auto it = futures_.begin(); it != futures_.end();) {
			auto future = *it;
			if (is_ready(future->future)) {
				it = futures_.erase(it);
				++count;

				auto ret = future->future.get();
				future->func(ret);
			} else {
				it++;
			}
		}
		return count;
	}

	void PushTask(int x, int y, const std::function<void(int)>& func) {
		auto future = std::make_shared<StCallback>();
		future->func = std::move(func);
		future->future = std::move(pool_.submit(ThreadProc, x, y));
		futures_.push_back(future);
	}

	bool Empty() { return futures_.empty(); }

private:
	static int ThreadProc(int x, int y) {
		int ret = x + y;
		int rand_ms = rand() % 10000;
		std::this_thread::sleep_for(std::chrono::milliseconds(rand_ms));
		return ret;
	}

	template <typename R>
	static bool is_ready(std::future<R> const& f) {
		return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}

	struct StCallback {
		std::future<int> future;
		std::function<void(int)> func;
	};

	std::list<std::shared_ptr<StCallback>> futures_;
	thread_pool::ThreadPool pool_;
};

int main() {
	srand(time(nullptr));
	CalPoolExample mgr;
	mgr.Init();

	for (int i = 0; i < 20; i++) {
		int x = rand() % 1000;
		int y = rand() % 3000;

		mgr.PushTask(x, y,
			[x, y](int ret) { printf("Calculate finished x:%d, y:%d, ret:%d\n", x, y, ret); });
	}

	while (true) {
		mgr.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (mgr.Empty()) {
			printf("done \n");
			break;
		}
	}

	return 0;
}
