/*
 *      dllist.h
 *
 *      Copyright 2007 Joey Durham <joey@engineering.ucsb.edu>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifndef DLLIST_H
#define DLLIST_H

#include <iostream>
#include <assert.h>

template <class tVARTYPE> class DLList;



template <class tVARTYPE> class DLLNode
{
	friend class DLList<tVARTYPE>;
	
	protected :
		DLLNode<tVARTYPE>* m_pNext;
		DLLNode<tVARTYPE>* m_pPrev;
		
	public :
		tVARTYPE m_data;
		
		DLLNode(const tVARTYPE& val)
		{
			m_data = val;
			
			m_pNext = NULL;
			m_pPrev = NULL;
		}
		virtual ~DLLNode() {}

};
template <class tVARTYPE> class DLList
{

public:

    DLList();
    virtual ~DLList();

    void clear();

    void insertAtBeginning(const tVARTYPE& val);
    void insertAtEnd(const tVARTYPE& val);
    void insertBefore(const tVARTYPE& val, DLLNode<tVARTYPE>* pThisNode);
    void insertAfter(const tVARTYPE& val, DLLNode<tVARTYPE>* pThisNode);
    DLLNode<tVARTYPE>* deleteNode(DLLNode<tVARTYPE>* pNode);
    void moveToEnd(DLLNode<tVARTYPE>* pThisNode);

    DLLNode<tVARTYPE>* next(DLLNode<tVARTYPE>* pNode) const;
    DLLNode<tVARTYPE>* prev(DLLNode<tVARTYPE>* pNode) const;

    DLLNode<tVARTYPE>* nodeNum(int iNum) const;

    int getLength() const
    {
        return m_iLength;
    }

    DLLNode<tVARTYPE>* head() const
    {
        return m_pHead;
    }

    DLLNode<tVARTYPE>* tail() const
    {
        return m_pTail;
    }

protected:
    int m_iLength;

    DLLNode<tVARTYPE>* m_pHead;
    DLLNode<tVARTYPE>* m_pTail;
};



//constructor
//resets local vars
template <class tVARTYPE>
inline DLList<tVARTYPE>::DLList()
{
    m_iLength = 0;

    m_pHead = NULL;
    m_pTail = NULL;
}


//Destructor
//resets local vars
template <class tVARTYPE>
inline DLList<tVARTYPE>::~DLList()
{
  clear();
}


template <class tVARTYPE>
inline void DLList<tVARTYPE>::clear()
{
    DLLNode<tVARTYPE>* pCurrNode;
    DLLNode<tVARTYPE>* pNextNode;

    pCurrNode = m_pHead;
    while (pCurrNode != NULL)
    {
        pNextNode = pCurrNode->m_pNext;
        delete pCurrNode;
        pCurrNode = pNextNode;
    }

    m_iLength = 0;

    m_pHead = NULL;
    m_pTail = NULL;
}


//inserts at the tail of the list
template <class tVARTYPE>
inline void DLList<tVARTYPE>::insertAtBeginning(const tVARTYPE& val)
{   
    DLLNode<tVARTYPE>* pNode;

    assert((m_pHead == NULL) || (m_iLength > 0));

    pNode = new DLLNode<tVARTYPE>(val);

    if (m_pHead != NULL)
    {
        m_pHead->m_pPrev = pNode;
        pNode->m_pNext = m_pHead;
        m_pHead = pNode;
    }
    else
    {
        m_pHead = pNode;
        m_pTail = pNode;
    }

    m_iLength++;
}


//inserts at the tail of the list
template <class tVARTYPE>
inline void DLList<tVARTYPE>::insertAtEnd(const tVARTYPE& val)
{   
    DLLNode<tVARTYPE>* pNode;

    assert((m_pHead == NULL) || (m_iLength > 0));

    pNode = new DLLNode<tVARTYPE>(val);

    if (m_pTail != NULL)
    {
        m_pTail->m_pNext = pNode;
        pNode->m_pPrev = m_pTail;
        m_pTail = pNode;
    }
    else
    {
        m_pHead = pNode;
        m_pTail = pNode;
    }

    m_iLength++;
}


//inserts before the specified node
template <class tVARTYPE>
inline void DLList<tVARTYPE>::insertBefore(const tVARTYPE& val, DLLNode<tVARTYPE>* pThisNode)
{
    DLLNode<tVARTYPE>* pNode;

    assert((m_pHead == NULL) || (m_iLength > 0));

    if ((pThisNode == NULL) || (pThisNode->m_pPrev == NULL))
    {
        insertAtBeginning(val);
        return;
    }

    pNode = new DLLNode<tVARTYPE>(val);

    pThisNode->m_pPrev->m_pNext = pNode;
    pNode->m_pPrev = pThisNode->m_pPrev;
    pThisNode->m_pPrev = pNode;
    pNode->m_pNext = pThisNode;

    m_iLength++;
}


//inserts after the specified node
template <class tVARTYPE>
inline void DLList<tVARTYPE>::insertAfter(const tVARTYPE& val, DLLNode<tVARTYPE>* pThisNode)
{
    DLLNode<tVARTYPE>* pNode;

    assert((m_pHead == NULL) || (m_iLength > 0));

    if ((pThisNode == NULL) || (pThisNode->m_pNext == NULL))
    {
        insertAtEnd(val);
        return;
    }

    pNode = new DLLNode<tVARTYPE>(val);

    pThisNode->m_pNext->m_pPrev = pNode;
    pNode->m_pNext              = pThisNode->m_pNext;
    pThisNode->m_pNext          = pNode;
    pNode->m_pPrev                    = pThisNode;

    m_iLength++;
}


template <class tVARTYPE>
inline DLLNode<tVARTYPE>* DLList<tVARTYPE>::deleteNode(DLLNode<tVARTYPE>* pNode)
{
    DLLNode<tVARTYPE>* pPrevNode;
    DLLNode<tVARTYPE>* pNextNode;

    assert(pNode != NULL);

    pPrevNode = pNode->m_pPrev;
    pNextNode = pNode->m_pNext;

    if ((pPrevNode != NULL) && (pNextNode != NULL))
    {
        pPrevNode->m_pNext = pNextNode;
        pNextNode->m_pPrev = pPrevNode;
    }
    else if (pPrevNode != NULL)
    {
        pPrevNode->m_pNext = NULL;
        m_pTail = pPrevNode;
    }
    else if (pNextNode != NULL)
    {
        pNextNode->m_pPrev = NULL;
        m_pHead = pNextNode;
    }
    else
    {
        m_pHead = NULL;
        m_pTail = NULL;
    }

    delete pNode;

    m_iLength--;

    return pNextNode;
}


template <class tVARTYPE>
inline void DLList<tVARTYPE>::moveToEnd(DLLNode<tVARTYPE>* pNode)
{
    DLLNode<tVARTYPE>* pPrevNode;
    DLLNode<tVARTYPE>* pNextNode;

    assert(pNode != NULL);

    if (getLength() == 1)
    {
    return;
    }

    if (pNode == m_pTail)
    {
        return;
    }

    pPrevNode = pNode->m_pPrev;
    pNextNode = pNode->m_pNext;

    if ((pPrevNode != NULL) && (pNextNode != NULL))
    {
        pPrevNode->m_pNext = pNextNode;
        pNextNode->m_pPrev = pPrevNode;
    }
    else if (pPrevNode != NULL)
    {
        pPrevNode->m_pNext = NULL;
        m_pTail = pPrevNode;
    }
    else if (pNextNode != NULL)
    {
        pNextNode->m_pPrev = NULL;
        m_pHead = pNextNode;
    }
    else
    {
        m_pHead = NULL;
        m_pTail = NULL;
    }

    pNode->m_pNext = NULL;
    m_pTail->m_pNext = pNode;
    pNode->m_pPrev = m_pTail;
    m_pTail = pNode;
}


template <class tVARTYPE>
inline DLLNode<tVARTYPE>* DLList<tVARTYPE>::next(DLLNode<tVARTYPE>* pNode) const
{
  assert(pNode != NULL);

  return pNode->m_pNext;
}


template <class tVARTYPE>
inline DLLNode<tVARTYPE>* DLList<tVARTYPE>::prev(DLLNode<tVARTYPE>* pNode) const
{
    assert(pNode != NULL);

    return pNode->m_pPrev;
}


template <class tVARTYPE>
inline DLLNode<tVARTYPE>* DLList<tVARTYPE>::nodeNum(int iNum) const
{
    DLLNode<tVARTYPE>* pNode;
    int iCount;

    iCount = 0;
    pNode = m_pHead;

    while (pNode != NULL)
    {
        if (iCount == iNum)
        {
            return pNode;
        }

        iCount++;
        pNode = pNode->m_pNext;
    }

    return NULL;
}


#endif    //LINKEDLIST_H
