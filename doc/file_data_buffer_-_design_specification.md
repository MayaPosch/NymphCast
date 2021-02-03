# File Data Buffer #

**Design specification document**


## Overview ##

The File Data Buffer (FDB) is a module which provides an interface (API) for the writing and reading of data, providing a FIFO-style circular buffer.

In addition, it provides functionality for managing meta information pertaining to the file whose data is being written and read (e.g. file size and byte offsets into file).

## Application Programming Interface (API) ##

The FDB's API methods can be divided into a few distinct groups:

- Initialisation.
- Clean-up.
- State.
- Writing data.
- Reading data.
- Seeking in the file.

**Initialisation**

```cpp
static bool init(uint32_t capacity);
```

Initialises the databuffer with a new capacity specified in bytes.

```cpp
static void setSessionHandle(uint32_t handle);
static uint32_t getSessionHandle();
```

Set or retrieve an associated session handle for this buffer.

```cpp
static void setFileSize(int64_t size);
```

Set the size of the file's data, in bytes.

```cpp
static void setSeekRequestCallback(SeekRequestCallback cb);
```

Set the callback for the seek function with signature `void(uint32_t session, int64_t offset)`.

```cpp
static void setDataRequestCondition(std::condition_variable* condition);
```

Provide a pointer to the condition variable to be called when more data is need by the buffer.

**Clean-up**

```cpp
static bool cleanup();
```

Deallocates any used resources.

```cpp
static bool reset();
```

Resets the buffer to its initial state, keeping - but emptying - the allocated buffer.

**State**

```cpp
static void setEof(bool eof);
static bool isEof();
```

Set or request the End-Of-File state of the file data.

```cpp
static bool start();
```

Causes the databuffer to request its first data via the data request condition variable.

```cpp
static void requestData();
```

Requests data via the data request condition variable and blocks until data has been received.

```cpp
static bool seeking();
```

Returns true if the buffer is currently in the middle of a seeking operation.

**Writing data**

```cpp
static uint32_t write(std::string &data);
```

Attempt to write the data contained in the STL string to the buffer. Returns the number of bytes written.

**Reading data**

```cpp
static uint32_t read(uint32_t len, uint8_t* bytes);
```

Attempt to read `len` bytes from the buffer, into the provided `bytes` buffer. Returns the number of bytes read.

**Seeking data**

```cpp
static int64_t seek(DataBufferSeek mode, int64_t offset);
```

Functions akin to `fseek()`. Mode can be one of:

Mode | Description
---|---
DB_SEEK_START 	| Seek from the beginning of the file.
 DB_SEEK_CURRENT 	| Seek from the current position in the file.
DB_SEEK_END		| Seek from the end of the file.

The provided offset is relative to the requested starting position. The buffer attempts to find the requested position first in the buffered data, otherwise it uses the seek request callback that was set before to obtain the data.

The new byte position in the file is returned on success, or -1 on error.

## Internal design ##

The DB has a number of views on the data:

- **Raw buffer**: includes the raw pointers into the allocated memory block.
- **Unread and free counters**: meta information on the states of parts of the buffer.
- **File byte indexes**: high-level counters to map the buffer data to the position in the file data.

**Raw buffer**

The raw buffer is a block of memory of size `capacity` that is allocated on the heap. It has a few raw pointers associated with it:

Pointer | Description
---|---
uint8_t* buffer | Pointer to the beginning of the buffer.
uint8_t* end	| Pointer to the last byte in the buffer.
uint8_t* front	| Pointer to the first data byte in the buffer.
uint8_t* back	| Pointer to the first free byte at the back of the buffer.
uint8_t* index 	| Pointer to the current read position.

The `buffer` pointer always points to the first byte in the buffer. The `end` pointer always points to the last byte.

The `front` pointer points to the first byte of data in the buffer, or the beginning of the buffer. The `back` pointer points to the first free byte of data in the buffer, or the end of the buffer.

The `index` pointer points to the first unread byte in the buffer, or the beginning of the buffer.

**Counters**

The unread & free bytes counters keep track of the number of unread and available (free) bytes. Unread bytes are bytes which have been written, but have not yet been read. Free bytes are bytes which have been read.

Newly written bytes are initially marked as 'unread', increasing the 'unread' count. Reading bytes decreases the 'unread' count. 

Counter | Description
---|---
uint32_t unread | Number of unread bytes in the buffer (total).
uint32_t free | Number of free bytes in the buffer (total).


**File indexes**

The file indexes keep track of which file data is in the buffer, i.e. which byte indexes are currently available inside the buffer, whether read or unread. This information is used during seeking operation to determine whether the requested new file offset is available in the buffer data.

Index | Description
---| ---
uint32_t byteIndexLow | Lowest byte index inside the buffer.
uint32_t byteIndexHigh | Highest byte index inside the buffer.