/* adlist.c - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdlib.h>
#include "adlist.h"
#include "zmalloc.h"

/* Create a new list. The created list can be freed with
 * AlFreeList(), but private value of every node need to be freed
 * by the user before to call AlFreeList().
 *
 * On error, NULL is returned. Otherwise the pointer to the new list. */

list *listCreate(void)
{
    struct list *list;

    if ((list = zmalloc(sizeof(*list))) == NULL)
        return NULL;
    list->head = list->tail = NULL;
    list->len = 0;
    list->dup = NULL;
    list->free = NULL;
    list->match = NULL;
    return list;
}

/* Free the whole list.
 *
 * This function can't fail. */
void listRelease(list *list)
{
    unsigned long len;
    listNode *current, *next;

    current = list->head;
    len = list->len;
    while(len--) {//逐步释放node 内存空间
        next = current->next;
        if (list->free) list->free(current->value);//释放
        zfree(current);
        current = next;//下一个节点
    }
    zfree(list);//释放list 指针内存空间
}

/* Add a new node to the list, to head, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */

//添加一个新节点到列表头部，包含指定的“值”指针的值。
 //添加节点到列表中
list *listAddNodeHead(list *list, void *value)
{
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)//尝试分配内存
        return NULL;
    node->value = value;
    if (list->len == 0) {//当前列表为空，把新增节点插入到头节点上，当前节点的prev next 都设为空
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
    //当前列表不为空，将当前节点插入到头节点
        node->prev = NULL;
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    list->len++;//长度+1
    return list;
}

/* Add a new node to the list, to tail, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
 //往尾端插入节点
list *listAddNodeTail(list *list, void *value)
{
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    node->value = value;
    if (list->len == 0) {////当前列表为空，把新增节点插入到头节点上，当前节点的prev next 都设为空
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
    //尾端插入节点
        node->prev = list->tail;
        node->next = NULL;
        list->tail->next = node;
        list->tail = node;
    }
    list->len++;
    return list;
}

//插入节点
/**
为list的某个节点old_node的after（大于0为前面新增，小于0为后面新增）
新增新值value，首先新建一个新节点node，判断是否有内存分配，如果有，则继续，如果没有，则返回NULL，退出方法。
这边新节点是用来存在输入参数中的value的，所以需要内存。接着根据after的值确定是在节点old_node的前面新增数据，
还是在节点old_node的后面新增数据，具体的是指针的调整。最后len加1，返回list。

**/
list *listInsertNode(list *list, listNode *old_node, void *value, int after) {
//在list的某个位置old_node的after(前后)插入value值
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    node->value = value;
    if (after) {//>0在old_node 之后插入
        node->prev = old_node;
        node->next = old_node->next;
        if (list->tail == old_node) {//如果插入是末尾
            list->tail = node;
        }
    } else {//<0 在old_ode 之前插入
        node->next = old_node;
        node->prev = old_node->prev;
        if (list->head == old_node) {//如果插入是头部
            list->head = node;
        }
    }
    if (node->prev != NULL) {//调整新插入的节点的前驱
        node->prev->next = node;
    }
    if (node->next != NULL) {//调整新插入节点的后置
        node->next->prev = node;
    }
    list->len++;
    return list;
}

/* Remove the specified node from the specified list.
 * It's up to the caller to free the private value of the node.
 *
 * This function can't fail. */
 //从链表list中删除某个节点node
void listDelNode(list *list, listNode *node)
{
	//如果该节点的前面存在节点
    if (node->prev)
        node->prev->next = node->next;
    else
        list->head = node->next;//删除是头
       //如果该节点的后置存在节点
    if (node->next)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;//删除的是尾
    if (list->free) list->free(node->value);////释放当前节点node的值
    zfree(node);//释放内存
    list->len--;
}

/* Returns a list iterator 'iter'. After the initialization every
 * call to listNext() will return the next element of the list.
 *
 * This function can't fail. */

 /*
  * 为给定链表创建一个迭代器，
  * 之后每次对这个迭代器调用 listNext 都返回被迭代到的链表节点
  *
  * direction 参数决定了迭代器的迭代方向：
  *  AL_START_HEAD ：从表头向表尾迭代
  *  AL_START_TAIL ：从表尾想表头迭代
  *
  * T = O(1)
  */
listIter *listGetIterator(list *list, int direction)
{
    listIter *iter;

    if ((iter = zmalloc(sizeof(*iter))) == NULL) return NULL;//分配内存
    if (direction == AL_START_HEAD)
        iter->next = list->head;//从头开始
    else
        iter->next = list->tail;//从尾开始
    iter->direction = direction;//设置迭代方向
    return iter;
}

/* Release the iterator memory */
void listReleaseIterator(listIter *iter) {
    zfree(iter);
}

/* Create an iterator in the list private iterator structure */
void listRewind(list *list, listIter *li) {//从头再来
    li->next = list->head;
    li->direction = AL_START_HEAD;
}

void listRewindTail(list *list, listIter *li) {//从尾再来
    li->next = list->tail;
    li->direction = AL_START_TAIL;
}

/* Return the next element of an iterator.
 * It's valid to remove the currently returned element using
 * listDelNode(), but not to remove other elements.
 *
 * The function returns a pointer to the next element of the list,
 * or NULL if there are no more elements, so the classical usage patter
 * is:
 *
 * iter = listGetIterator(list,<direction>);
 * while ((node = listNext(iter)) != NULL) {
 *     doSomethingWith(listNodeValue(node));
 * }
 *
 * */
//迭代链表
listNode *listNext(listIter *iter)
{
    listNode *current = iter->next;

    if (current != NULL) {
        if (iter->direction == AL_START_HEAD)
            iter->next = current->next;
        else
            iter->next = current->prev;
    }
    return current;
}

/* Duplicate the whole list. On out of memory NULL is returned.
 * On success a copy of the original list is returned.
 *
 * The 'Dup' method set with listSetDupMethod() function is used
 * to copy the node value. Otherwise the same pointer value of
 * the original node is used as value of the copied node.
 *
 * The original list both on success or error is never modified. */
//fixme
 //拷贝队列
list *listDup(list *orig)
{
    list *copy;
    listIter *iter;
    listNode *node;

    if ((copy = listCreate()) == NULL)//创建一个新的队列
        return NULL;
    copy->dup = orig->dup;//拷贝函数
    copy->free = orig->free;
    copy->match = orig->match;
    iter = listGetIterator(orig, AL_START_HEAD);//获取迭代器
    while((node = listNext(iter)) != NULL) {//逐步拷贝队列中的值
        void *value;
        if (copy->dup) {//如果设置了拷贝函数，
            value = copy->dup(node->value);
            if (value == NULL) {
                listRelease(copy);
                listReleaseIterator(iter);
                return NULL;
            }
        } else//直接进行值拷贝
            value = node->value;
        if (listAddNodeTail(copy, value) == NULL) {//往尾端插入节点
            //如果失败
            listRelease(copy);//释放copy 内存
            listReleaseIterator(iter);//释放迭代器内存
            return NULL;
        }
    }
    listReleaseIterator(iter);
    return copy;
}

/* Search the list for a node matching a given key.
 * The match is performed using the 'match' method
 * set with listSetMatchMethod(). If no 'match' method
 * is set, the 'value' pointer of every node is directly
 * compared with the 'key' pointer.
 *
 * On success the first matching node pointer is returned
 * (search starts from head). If no matching node exists
 * NULL is returned. */
 /*
  * 查找链表 list 中值和 key 匹配的节点。
  *
  * 对比操作由链表的 match 函数负责进行，
  * 如果没有设置 match 函数，
  * 那么直接通过对比值的指针来决定是否匹配。
  *
  * 如果匹配成功，那么第一个匹配的节点会被返回。
  * 如果没有匹配任何节点，那么返回 NULL 。
  *
  * T = O(N)
  */

listNode *listSearchKey(list *list, void *key)
{
    listIter *iter;
    listNode *node;

// 迭代整个链表
    iter = listGetIterator(list, AL_START_HEAD);
    while((node = listNext(iter)) != NULL) {
        if (list->match) { //通过match函数 对比
            if (list->match(node->value, key)) {
                listReleaseIterator(iter);//存在
                return node;
            }
        } else {
            if (key == node->value) {//通过key指针进行对比
                listReleaseIterator(iter);//存在
                return node;
            }
        }
    }
    listReleaseIterator(iter);//释放迭代器
    return NULL; //未找到
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range NULL is returned. */
listNode *listIndex(list *list, long index) {
    listNode *n;

    if (index < 0) {
        index = (-index)-1;
        n = list->tail;
        while(index-- && n) n = n->prev;
    } else {
        n = list->head;
        while(index-- && n) n = n->next;
    }
    return n;
}

/* Rotate the list removing the tail node and inserting it to the head. */
void listRotate(list *list) {
    listNode *tail = list->tail;

    if (listLength(list) <= 1) return;

    /* Detach current tail */
    list->tail = tail->prev;
    list->tail->next = NULL;
    /* Move it as head */
    list->head->prev = tail;
    tail->prev = NULL;
    tail->next = list->head;
    list->head = tail;
}
