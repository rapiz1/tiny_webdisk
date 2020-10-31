# tiny_webdisk

It's my toy project of writing a web server with epoll.

DON'T TREAT ANY CODE HERE SERIOUSLY AND DON'T USE IT ON YOUR SERVER. Neither did I read the rfc nor any effort were made to keep the code clean, efficent or safe.

USAGE:

In terminal,
```
mkdir disk
./tiny_webdisk
```

Then it will listen to 0.0.0.0:8080 and serve static content under working directory.

Accessing [http://localhost:8080](http://localhost:8080), you will see the landing page.

Via `GET /path/to/file` you will get `work_dir/path/to/file`. You can do this simply by appending path to the hostname, like [http://localhost:8080/somefile](http://localhost:8080/somefile) *(Assume that you have `somefile` under your working directory)*.

And `POST /path/to/file` will upload the content to `work_dir/path/to/file`.

No web interface for the POST usage. Use whatever tool you like.
