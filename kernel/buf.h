/*
 * @Author: qiujingbao qiujingbao@qq.com
 * @Date: 2023-04-11 11:26:44
 * @LastEditors: qiujingbao qiujingbao@qq.com
 * @LastEditTime: 2023-04-16 16:21:06
 * @FilePath: /xv6-labs-2020/kernel/buf.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
  uint time; //least used time
};

