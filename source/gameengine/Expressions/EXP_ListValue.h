/*
 * ListValue.h: interface for the CListValue class.
 * Copyright (c) 1996-2000 Erwin Coumans <coockie@acm.org>
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Erwin Coumans makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

/** \file EXP_ListValue.h
 *  \ingroup expressions
 */

#ifndef __EXP_LISTVALUE_H__
#define __EXP_LISTVALUE_H__

#include "EXP_BaseListValue.h"
#include "EXP_BoolValue.h"

#include <functional>

#include <algorithm>

template <class ItemType>
class CListValue : public CBaseListValue
{
public:
	class const_iterator
	{
	public:
		VectorTypeConstIterator m_it;

		const_iterator(VectorTypeConstIterator it)
		: m_it(it)
		{
		}

		inline void operator++()
		{
			++m_it;
		}

		inline ItemType *operator*() const
		{
			return static_cast<ItemType *>(*m_it);
		}
	};

	CListValue()
	{
	}
	virtual ~CListValue()
	{
	}

	virtual CListValue<ItemType> *GetReplica()
	{
		CListValue<ItemType> *replica = new CListValue<ItemType>(*this);

		replica->ProcessReplica();

		replica->m_bReleaseContents = true; // For copy, complete array is copied for now...
		// Copy all values.
		const int numelements = m_pValueArray.size();
		replica->m_pValueArray.resize(numelements);
		for (unsigned int i = 0; i < numelements; i++) {
			replica->m_pValueArray[i] = m_pValueArray[i]->GetReplica();
		}

		return replica;
	}

	void Add(ItemType *value)
	{
		CBaseListValue::Add(value);
	}

	void Insert(unsigned int i, ItemType *value)
	{
		CBaseListValue::Insert(i, value);
	}

	ItemType *FindIf(std::function<bool (ItemType *)> function)
	{
		for (CValue *val : m_pValueArray) {
			ItemType *item = static_cast<ItemType *>(val);
			if (function(item)) {
				return item;
			}
		}
		return nullptr;
	}

	void MergeList(CListValue<ItemType> *otherlist)
	{
		const unsigned int numelements = GetCount();
		const unsigned int numotherelements = otherlist->GetCount();

		Resize(numelements + numotherelements);

		for (int i = 0; i < numotherelements; i++) {
			SetValue(i + numelements, CM_AddRef(otherlist->GetValue(i)));
		}
	}
	bool CheckEqual(ItemType *first, ItemType *second)
	{
		return CBaseListValue::CheckEqual(first, second);
	}

	bool SearchValue(ItemType *val) const
	{
		return CBaseListValue::SearchValue(val);
	}
	ItemType *FindValue(const std::string& name) const
	{
		return static_cast<ItemType *>(CBaseListValue::FindValue(name));
	}

	bool RemoveValue(ItemType *val)
	{
		return CBaseListValue::RemoveValue(val);
	}

	void SetValue(int i, ItemType *val)
	{
		CBaseListValue::SetValue(i, val);
	}
	ItemType *GetValue(int i)
	{
		return static_cast<ItemType *>(CBaseListValue::GetValue(i));
	}

	ItemType *GetFront()
	{
		return static_cast<ItemType *>(m_pValueArray.front());
	}
	ItemType *GetBack()
	{
		return static_cast<ItemType *>(m_pValueArray.back());
	}

	const_iterator begin()
	{
		return const_iterator(m_pValueArray.begin());
	}
	const_iterator end()
	{
		return const_iterator(m_pValueArray.end());
	}
};

template<class ItemType>
typename CListValue<ItemType>::const_iterator begin(CListValue<ItemType> *list)
{
	return list->begin();
}

template<class ItemType>
typename CListValue<ItemType>::const_iterator end(CListValue<ItemType> *list)
{
	return list->end();
}

template <class Iterator, typename = decltype(std::declval<Iterator>().m_it) >
bool operator!=(const Iterator& it1, const Iterator& it2)
{
	return it1.m_it != it2.m_it;
}

#endif  // __EXP_LISTVALUE_H__

