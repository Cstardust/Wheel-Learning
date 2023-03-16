package main

import (
	"fmt"
	"sync"
)

//
// Several solutions to the crawler exercise from the Go tutorial
// https://tour.golang.org/concurrency/10
//

//
// Concurrent crawler with shared state and Mutex
//

type fetchState struct {
	mu      sync.Mutex
	fetched map[string]bool
}

func ConcurrentMutex(url string, fetcher Fetcher, f *fetchState) {
	f.mu.Lock()
	already := f.fetched[url]
	f.fetched[url] = true
	f.mu.Unlock()

	if already {
		return
	}

	urls, err := fetcher.Fetch(url)
	if err != nil {
		return
	}
	var done sync.WaitGroup
	for _, u := range urls {
		done.Add(1)
		go func(u string) {
			defer done.Done()
			ConcurrentMutex(u, fetcher, f)
		}(u)
	}
	done.Wait()
	return
}

func makeState() *fetchState {
	f := &fetchState{}
	f.fetched = make(map[string]bool)
	return f
}

//
// Concurrent crawler with channels
//

func worker(url string, ch chan []string, fetcher Fetcher) {
	urls, err := fetcher.Fetch(url)
	if err != nil {
		ch <- []string{}
	} else {
		ch <- urls
	}
}

func coordinator(ch chan []string, fetcher Fetcher) {
	n := 1
	fetched := make(map[string]bool)
	for urls := range ch {
		for _, u := range urls {
			if fetched[u] == false {
				fetched[u] = true
				n += 1
				go worker(u, ch, fetcher)
			}
		}
		n -= 1
		if n == 0 {
			break
		}
	}
}

func ConcurrentChannel(url string, fetcher Fetcher) {
	ch := make(chan []string)
	go func() {
		ch <- []string{url}
	}()
	coordinator(ch, fetcher)
}

//
// Serial crawler
//

//  当前已经查询到url，fetcher继续通过url进行查询, fetched是已经查询到的结果集
//  根据fecthed可以看出，go里面都是传递引用？？
func Serial(url string, fetcher Fetcher, fetched map[string]bool) {
	//  如果当前结果集中已经存在url，则返回
	//  爬虫的关键：判断什么时候结束爬取. 那么就要避免重复查询
	if fetched[url] {
		return
	}
	//  标记已经爬取到url
	fetched[url] = true
	//  (模拟)fectcher爬取到url page上存在的其他url
	urls, err := fetcher.Fetch(url)
	if err != nil { //  不存在，则结束dfs
		return
	}
	//  根据本轮查询到的结果集urls , dfs继续查询
	for _, u := range urls {
		Serial(u, fetcher, fetched)
	}
	return
}

//
// main
//

func main() {
	fmt.Printf("=== Serial===\n")
	Serial("http://golang.org/", fetcher, make(map[string]bool))

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
