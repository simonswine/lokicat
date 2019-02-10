# Lokicat

Slim log forwarder for loki, pre alpha, PoC


```
$ cat sample-busybox-logread.txt |./lokicat --url=http://localhost:3100 --input=logread                                             130 ↵
input=logread loki_url=http://localhost:3100/api/prom/push
retrieved entry of length 79 :
retrieved entry of length 106 :
retrieved entry of length 130 :
retrieved entry of length 83 :
retrieved entry of length 178 :
snappy compression bytes_before=646 -> bytes_after=502
writing 502 serialized + compressed bytes
*   Trying ::1...
* TCP_NODELAY set
* Connected to localhost (::1) port 3100 (#0)
> POST /api/prom/push HTTP/1.1
Host: localhost:3100
User-Agent: lokicat/0.1.0
Accept: */*
Content-Type: application/octet-stream
Content-Length: 502

* upload completely sent off: 502 out of 502 bytes
< HTTP/1.1 204 No Content
< Date: Sun, 10 Feb 2019 19:07:39 GMT
< 
* Connection #0 to host localhost left intact
```
