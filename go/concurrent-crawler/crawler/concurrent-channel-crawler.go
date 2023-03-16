package main

import "fmt"

//
// Concurrent crawler with channels
//

//  通过channel而不是共享内存
//  worker和master之间不shared任何对象
//  channel在内部维护了mutex，来保证多个thread使用他时不会发生data race
//  fecther爬取url page上的所有urls,然后通过channel ch传送给master,也叫coordinator所在thread
func worker(url string, ch chan []string, fetcher Fetcher) {
	urls, err := fetcher.Fetch(url)
	if err != nil {
		ch <- []string{}
	} else {
		ch <- urls //  worker将urls写入channel
	}
}

func master(ch chan []string, fetcher Fetcher) {
	n := 1
	fetched := make(map[string]bool) //  fetched 为master私有变量，而非共享内存
	for urls := range ch {           //  阻塞遍历worker送入channel中的string[]
		for _, u := range urls { //  遍历strings[]，逐个开启thread去爬取url
			if fetched[u] == false {
				fetched[u] = true
				n += 1
				go worker(u, ch, fetcher)
			}
		}
		n -= 1
		if n == 0 { //  2. why 0?
			break
		}
	}
}

func ConcurrentChannel(url string, fetcher Fetcher) {
	ch := make(chan []string)
	go func() { //  1. 为啥不能注释掉?	直接在main thread写channel ? 写channel是阻塞的 ? 不至于吧 ?
		ch <- []string{url}
	}()
	master(ch, fetcher)
}

func main() {
	fmt.Printf("=== ConcurrentChannel ===\n")
	ConcurrentChannel("http://golang.org/", fetcher)
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
type fakeFetcher map[string]*fakeResult //  3. 这个*fakeResult是啥意思??
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
	"http://golang.org/": &fakeResult{ //  4. 这个&是干什么的 ?
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

//  5. go 传参.
