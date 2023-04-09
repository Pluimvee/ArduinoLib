#include <LittleFS.h>
#include <ArduinoJson.h>
#include <FS.h>
#include "FlashStack.h"

#if (0)
  #define DEBUG(...)    Serial.printf(__VA_ARGS__)
  #define INFO(...)     Serial.printf(__VA_ARGS__)
  #define ERROR(...)    Serial.printf(__VA_ARGS__)
  #define BLOCK_SIZE    1024
#else
  #define DEBUG(...)
  #define INFO(...) 
  #define ERROR(...)
  #define BLOCK_SIZE    info.blockSize
#endif

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool FlashStack::begin(const char* cache_name, size_t record_size, size_t cache_size)
{
  if (!LittleFS.begin())
     return false;
  FSInfo info;
  LittleFS.info(info);
  INFO("File system mounted: blocksize %d, total %d, used %d\n", info.blockSize, info.totalBytes, info.usedBytes);

  _file = LittleFS.open(cache_name, "r+");    // open for reading and writing, w+ would truncate the file, a+ would have all output appended
  if (!_file)
    return false; //_err("Failed to open caching file for writing");
  INFO("File \'%s\' opened\n", cache_name);

  if (_file.size() < sizeof(_meta))
  {
    // cache_size is optional, and calcultated using a file block
    // substracting the meta header and divided by the record size
    if (cache_size == 0)     
      cache_size = (BLOCK_SIZE - sizeof(_meta)) / record_size;

    INFO("Creating new cache for records holding %d bytes. Max nr of records %d\n", record_size, cache_size);
    _meta.size  = record_size;
    _meta.max   = cache_size;
    _meta.head  = 0;
    _meta.tail  = 0;
    _meta.count = 0;

    return _flush();
  }
  else
  { 
    if (!_file.seek(0) ||
        _file.read((uint8_t*)&_meta, sizeof(_meta)) != sizeof(_meta))
      return false;
    DEBUG("Meta data read with size %d, max %d, head %d, tail %d, count %d\n", _meta.size, _meta.max, _meta.head, _meta.tail, _meta.count);
    if (_meta.size != record_size)
    {
      INFO("Record size changed !! Wipe existing cache and start over with with a new cache\n");
      _file.truncate(0);  // truncate file to 0 bytes
      _file.close();      // close the file as the call to begin will re-open
      return begin(cache_name, record_size, cache_size);
    }
  }
  return true; 
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool FlashStack::_seek(uint16_t index) const {
  return _file.seek(sizeof(_meta) + index * _meta.size);
}

bool FlashStack::_flush() {
  if (_file.seek(0) &&
      _file.write((uint8_t*) &_meta, sizeof(_meta)) == sizeof(_meta))
  {
    _file.flush();
    INFO("Meta flushed [size %d, max %d, head %d, tail %d, count %d]\n", _meta.size, _meta.max, _meta.head, _meta.tail, _meta.count);
    return true;
  }
  return false;
}

bool FlashStack::reset() // reset head, tail and cache in existing cache
{
  _meta.head = _meta.tail = _meta.count = 0;
  return _flush();
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool FlashStack::push(const void *record, bool force)
{
  if (isfull() && !force)
    return false;
  DEBUG("Writing record at %d\n", _meta.head);
  if (!_seek(_meta.head))
    return false;
  if (_file.write((uint8_t*) record, _meta.size) != _meta.size)
    return false;

  // If the cache is full (and force is set) we have just written over the tail record 
  // we must increase the tail and rollover if needed
  if (isfull()) {
    if (++_meta.tail == max())
      _meta.tail = 0;
  }
  // change the write pointer, rolover if reached max
  if (++_meta.head == max())
    _meta.head = 0;
  if (++_meta.count > max())
    _meta.count = max(); 

  return _flush();
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool FlashStack::peek(void *record) const
{
  DEBUG("Peeking record at %d\n", _meta.tail);
  return  !isempty() &&
          _seek(_meta.tail) &&
          _file.read((uint8_t*) record, _meta.size) == _meta.size;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool FlashStack::pop(void *record)
{
  DEBUG("Retrieving record at %d\n", _meta.tail);
  if (record && !peek(record))  // if record is given, retrieve the record using the peek method
    return false;
  
  _meta.count--;                // the peek method checked for empty cache (count > 0)
  if (++_meta.tail == max())    // add moving the read pointer
    _meta.tail = 0;

  return _flush();
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
