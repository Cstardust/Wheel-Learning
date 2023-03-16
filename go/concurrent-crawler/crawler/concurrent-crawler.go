package main

import (
	"fmt"
	"sync"
)

//
// Concurrent crawler with shared state and Mutex
//
//  通过shared-data和lock来实现. 所有thread共享这个fetched-map
//  临界区: 结果集合 fectched. 通过mutx保护.
type fetchState struct {
	mu      sync.Mutex
	fetched map[string]bool
}

//  go正常传参的时候，到底传的是引用、值、还是指针?
//  为什么fecthState需要指针？之前串行化的map就不需要。不都是要传递地址吗?
//	go里，如果你声明一个对象的类型为map，他其实就是一个指向heap中某些数据的指针
func ConcurrentMutex(url string, fetcher Fetcher, f *fetchState) {
	//  将查询结果加入集合
	f.mu.Lock()
	already := f.fetched[url]
	f.fetched[url] = true
	f.mu.Unlock()
	//  如果之前已经查询过，那么return
	if already {
		return
	}
	//  模拟爬虫爬取到url page上的url
	urls, err := fetcher.Fetch(url)
	if err != nil {
		return
	}
	var done sync.WaitGroup
	//  对于查询到的每一个新url，开启一个thread去查询
	for _, u := range urls {
		done.Add(1)         //  done 计数器 ++
		go func(u string) { //  不想通过引用引用外部变量。于是通过一个传参的拷贝。
			defer done.Done()              //  done 计数器-- (函数结束时)
			ConcurrentMutex(u, fetcher, f) //  这里的fecther,f 是 通过引用引用外部变量
		}(u)
	}
	//  每一个thread等待他开启的thread结束 再返回. 不然main函数早就结束了.
	done.Wait() // done阻塞直到waitgroup计数器恢复为0
	return
}

func makeState() *fetchState {
	f := &fetchState{}
	f.fetched = make(map[string]bool)
	return f
}

//
// main
//

func main() {
	fmt.Printf("=== ConcurrentMutex ===\n")
	ConcurrentMutex("http://golang.org/", fetcher, makeState())

	// fmt.Printf("=== ConcurrentChannel ===\n")
	// ConcurrentChannel("http://golang.org/", fetcher)
}

//
// Fetcher
//
//  Interface Fetcher : 返回在某page(url)上，爬取到的其他urls
type Fetcher interface {
	// Fetch returns a slice of URLs found on the page.
	Fetch(url string) (urls []string, err error)
}

//  假爬取结果的集合
// fakeFetcher is Fetcher that returns canned results.
type fakeFetcher map[string]*fakeResult //  这个*fakeResult是啥意思??
// key是string value是fakeResult*
// map[key-type]val-type
//  string是请求的url吗
//  value是http响应吗

//  假爬取结果
type fakeResult struct {
	body string
	urls []string
}

func (f fakeFetcher) Fetch(url string) ([]string, error) {
	//  目的：在fakeFecther中，找出url对应的fakeResult.
	//  如果找到，则打印found，返回fakeResult的所有urls
	//  如果没找到，则打印missing，返回fakeResult
	if res, ok := f[url]; ok { //  ok代表k-v是否存在	res是fakeResult*
		fmt.Printf("found:   %s\n", url)
		return res.urls, nil
	}
	fmt.Printf("missing: %s\n", url)
	return nil, fmt.Errorf("not found: %s", url)
}

// fetcher is a populated fakeFetcher.
//  一个假的根据string(原url)获取到的相关url的集合
var fetcher = fakeFetcher{
	//  根据http://golang.org/,爬到了The Go Programming Language相关的http://golang.org/pkg/,http://golang.org/cmd/
	"http://golang.org/": &fakeResult{ //  这个&是干什么的 ?
		"The Go Programming Language",
		[]string{
			"http://golang.org/pkg/",
			"http://golang.org/cmd/",
		},
	},
	"http://golang.org/pkg/": &fakeResult{
		"Packages",
		[]string{
			"http://golang.org/",
			"http://golang.org/cmd/",
			"http://golang.org/pkg/fmt/",
			"http://golang.org/pkg/os/",
		},
	},
	"http://golang.org/pkg/fmt/": &fakeResult{
		"Package fmt",
		[]string{
			"http://golang.org/",
			"http://golang.org/pkg/",
		},
	},
	"http://golang.org/pkg/os/": &fakeResult{
		"Package os",
		[]string{
			"http://golang.org/",
			"http://golang.org/pkg/",
		},
	},
}
