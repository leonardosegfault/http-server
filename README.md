This is my try of RFC 2616 implementation as part of my study of C language and
Linux API.

Although it's functional, I'd probably not use it in production due to the lack
of enough tests.

# TODO
- [ ] better concurrency: the data receiving/sending needs to be more efficient
in concurrency, as there's nothing preventing a client socket to make the thread
busy.

- [ ] implement common MIME types:
https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/MIME_types/Common_types

- [ ] caching.