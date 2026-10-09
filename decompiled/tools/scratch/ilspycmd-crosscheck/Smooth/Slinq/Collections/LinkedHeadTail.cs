using System;
using System.Collections.Generic;
using Smooth.Collections;
using Smooth.Delegates;
using Smooth.Slinq.Context;

namespace Smooth.Slinq.Collections
{
	public struct LinkedHeadTail<T> : IEquatable<LinkedHeadTail<T>>
	{
		public Linked<T> head;

		public Linked<T> tail;

		public int count;

		public LinkedHeadTail(T value)
			: this(Linked<T>.Borrow(value))
		{
		}

		public LinkedHeadTail(Linked<T> head)
		{
			if (head == null)
			{
				this.head = null;
				tail = null;
				count = 0;
				return;
			}
			this.head = head;
			tail = head;
			count = 1;
			while (tail.next != null)
			{
				tail = tail.next;
				count++;
			}
		}

		public override bool Equals(object other)
		{
			if (other is LinkedHeadTail<T>)
			{
				return Equals((LinkedHeadTail<T>)other);
			}
			return false;
		}

		public bool Equals(LinkedHeadTail<T> other)
		{
			return head == other.head;
		}

		public override int GetHashCode()
		{
			return head.GetHashCode();
		}

		public static bool operator ==(LinkedHeadTail<T> lhs, LinkedHeadTail<T> rhs)
		{
			return lhs.head == rhs.head;
		}

		public static bool operator !=(LinkedHeadTail<T> lhs, LinkedHeadTail<T> rhs)
		{
			return lhs.head != rhs.head;
		}

		public void Append(T value)
		{
			Linked<T> next = Linked<T>.Borrow(value);
			if (head == null)
			{
				head = next;
			}
			else
			{
				tail.next = next;
			}
			tail = next;
			count++;
		}

		public void Append(Linked<T> node)
		{
			if (node != null)
			{
				if (head == null)
				{
					head = node;
				}
				else
				{
					tail.next = node;
				}
				tail = node;
				count++;
				while (tail.next != null)
				{
					tail = tail.next;
					count++;
				}
			}
		}

		public void Append(LinkedHeadTail<T> other)
		{
			if (other.count != 0)
			{
				if (head == null)
				{
					head = other.head;
					tail = other.tail;
					count = other.count;
				}
				else
				{
					tail.next = other.head;
					tail = other.tail;
					count += other.count;
				}
			}
		}

		public void DisposeInBackground()
		{
			if (head != null)
			{
				head.DisposeInBackground();
			}
			head = null;
			tail = null;
			count = 0;
		}

		public void Dispose()
		{
			if (head != null)
			{
				head.Dispose();
			}
			head = null;
			tail = null;
			count = 0;
		}

		public Slinq<T, LinkedContext<T>> SlinqAndKeep()
		{
			return LinkedContext<T>.Slinq(this, release: false);
		}

		public Slinq<T, LinkedContext<T>> SlinqAndKeep(BacktrackDetector bd)
		{
			return LinkedContext<T>.Slinq(this, bd, release: false);
		}

		public Slinq<T, LinkedContext<T>> SlinqAndDispose()
		{
			Slinq<T, LinkedContext<T>> result = LinkedContext<T>.Slinq(this, release: true);
			head = null;
			tail = null;
			count = 0;
			return result;
		}

		public Lookup<K, T> AddTo<K>(Lookup<K, T> lookup, DelegateFunc<T, K> selector)
		{
			while (head != null)
			{
				Linked<T> linked = head;
				head = head.next;
				linked.next = null;
				lookup.Add(selector(linked.value), linked);
			}
			tail = null;
			count = 0;
			return lookup;
		}

		public Lookup<K, T> AddTo<K, P>(Lookup<K, T> lookup, DelegateFunc<T, P, K> selector, P parameter)
		{
			while (head != null)
			{
				Linked<T> linked = head;
				head = head.next;
				linked.next = null;
				lookup.Add(selector(linked.value, parameter), linked);
			}
			tail = null;
			count = 0;
			return lookup;
		}

		public Lookup<K, T> ToLookup<K>(DelegateFunc<T, K> selector)
		{
			return AddTo(Lookup<K, T>.Borrow(Smooth.Collections.EqualityComparer<K>.Default), selector);
		}

		public Lookup<K, T> ToLookup<K>(DelegateFunc<T, K> selector, IEqualityComparer<K> comparer)
		{
			return AddTo(Lookup<K, T>.Borrow(comparer), selector);
		}

		public Lookup<K, T> ToLookup<K, P>(DelegateFunc<T, P, K> selector, P parameter)
		{
			return AddTo(Lookup<K, T>.Borrow(Smooth.Collections.EqualityComparer<K>.Default), selector, parameter);
		}

		public Lookup<K, T> ToLookup<K, P>(DelegateFunc<T, P, K> selector, P parameter, IEqualityComparer<K> comparer)
		{
			return AddTo(Lookup<K, T>.Borrow(comparer), selector, parameter);
		}
	}
	public struct LinkedHeadTail<K, T> : IEquatable<LinkedHeadTail<K, T>>
	{
		public Linked<K, T> head;

		public Linked<K, T> tail;

		public int count;

		public LinkedHeadTail(K key, T value)
			: this(Linked<K, T>.Borrow(key, value))
		{
		}

		public LinkedHeadTail(Linked<K, T> head)
		{
			if (head == null)
			{
				this.head = null;
				tail = null;
				count = 0;
				return;
			}
			this.head = head;
			tail = head;
			count = 1;
			while (tail.next != null)
			{
				tail = tail.next;
				count++;
			}
		}

		public override bool Equals(object other)
		{
			if (other is LinkedHeadTail<K, T>)
			{
				return Equals((LinkedHeadTail<K, T>)other);
			}
			return false;
		}

		public bool Equals(LinkedHeadTail<K, T> other)
		{
			return head == other.head;
		}

		public override int GetHashCode()
		{
			return head.GetHashCode();
		}

		public static bool operator ==(LinkedHeadTail<K, T> lhs, LinkedHeadTail<K, T> rhs)
		{
			return lhs.head == rhs.head;
		}

		public static bool operator !=(LinkedHeadTail<K, T> lhs, LinkedHeadTail<K, T> rhs)
		{
			return lhs.head != rhs.head;
		}

		public void Append(K key, T value)
		{
			Linked<K, T> next = Linked<K, T>.Borrow(key, value);
			if (head == null)
			{
				head = next;
			}
			else
			{
				tail.next = next;
			}
			tail = next;
			count++;
		}

		public void Append(Linked<K, T> node)
		{
			if (head == null)
			{
				head = node;
			}
			else
			{
				tail.next = node;
			}
			tail = node;
			count++;
			while (tail.next != null)
			{
				tail = tail.next;
				count++;
			}
		}

		public void Append(LinkedHeadTail<K, T> other)
		{
			if (other.count != 0)
			{
				if (head == null)
				{
					head = other.head;
					tail = other.tail;
					count = other.count;
				}
				else
				{
					tail.next = other.head;
					tail = other.tail;
					count += other.count;
				}
			}
		}

		public void DisposeInBackground()
		{
			if (head != null)
			{
				head.DisposeInBackground();
			}
			head = null;
			tail = null;
			count = 0;
		}

		public void Dispose()
		{
			if (head != null)
			{
				head.Dispose();
			}
			head = null;
			tail = null;
			count = 0;
		}

		public Slinq<T, LinkedContext<K, T>> SlinqAndKeep()
		{
			return LinkedContext<K, T>.Slinq(this, release: false);
		}

		public Slinq<T, LinkedContext<K, T>> SlinqAndKeep(BacktrackDetector bd)
		{
			return LinkedContext<K, T>.Slinq(this, bd, release: false);
		}

		public Slinq<T, LinkedContext<K, T>> SlinqAndDispose()
		{
			Slinq<T, LinkedContext<K, T>> result = LinkedContext<K, T>.Slinq(this, release: true);
			head = null;
			tail = null;
			count = 0;
			return result;
		}
	}
}
