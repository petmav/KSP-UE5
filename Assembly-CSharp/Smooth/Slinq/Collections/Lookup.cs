using System;
using System.Collections.Generic;
using Smooth.Delegates;
using Smooth.Dispose;
using Smooth.Slinq.Context;
using UnityEngine;

namespace Smooth.Slinq.Collections
{
	public class Lookup<K, T> : IDisposable
	{
		private static readonly Stack<Lookup<K, T>> pool = new Stack<Lookup<K, T>>();

		public LinkedHeadTail<K> keys;

		public readonly Dictionary<K, LinkedHeadTail<T>> dictionary;

		private Lookup()
		{
		}

		private Lookup(IEqualityComparer<K> comparer)
		{
			dictionary = new Dictionary<K, LinkedHeadTail<T>>(comparer);
			keys = default(LinkedHeadTail<K>);
		}

		public static Lookup<K, T> Borrow(IEqualityComparer<K> comparer)
		{
			lock (pool)
			{
				return (pool.Count > 0) ? pool.Pop() : new Lookup<K, T>(comparer);
			}
		}

		public void DisposeInBackground()
		{
			DisposalQueue.Enqueue(this);
		}

		public void Dispose()
		{
			LinkedHeadTail<T> linkedHeadTail = default(LinkedHeadTail<T>);
			for (Linked<K> linked = keys.head; linked != null; linked = linked.next)
			{
				linkedHeadTail.Append(RemoveValues(linked.value));
			}
			if (dictionary.Count > 0)
			{
				Debug.LogWarning("Lookup had dictionary keys that were not in the key list.");
				foreach (LinkedHeadTail<T> value in dictionary.Values)
				{
					linkedHeadTail.Append(value);
				}
				dictionary.Clear();
			}
			keys.Dispose();
			linkedHeadTail.Dispose();
			lock (pool)
			{
				pool.Push(this);
			}
		}

		public void Add(K key, T value)
		{
			if (!dictionary.TryGetValue(key, out var value2))
			{
				keys.Append(key);
			}
			value2.Append(value);
			dictionary[key] = value2;
		}

		public void Add(K key, Linked<T> value)
		{
			if (!dictionary.TryGetValue(key, out var value2))
			{
				keys.Append(key);
			}
			value2.Append(value);
			dictionary[key] = value2;
		}

		public void Add(K key, LinkedHeadTail<T> values)
		{
			if (dictionary.TryGetValue(key, out var value))
			{
				value.Append(values);
				dictionary[key] = value;
			}
			else
			{
				keys.Append(key);
				dictionary[key] = values;
			}
		}

		public LinkedHeadTail<T> GetValues(K key)
		{
			dictionary.TryGetValue(key, out var value);
			return value;
		}

		public LinkedHeadTail<T> RemoveValues(K key)
		{
			if (dictionary.TryGetValue(key, out var value))
			{
				dictionary.Remove(key);
			}
			return value;
		}

		public Lookup<K, T> SortKeys(Comparison<K> comparison, bool ascending)
		{
			keys = Linked.Sort(keys, comparison, ascending);
			return this;
		}

		public LinkedHeadTail<T> FlattenAndDispose()
		{
			LinkedHeadTail<T> result = default(LinkedHeadTail<T>);
			for (Linked<K> linked = keys.head; linked != null; linked = linked.next)
			{
				result.Append(RemoveValues(linked.value));
			}
			keys.DisposeInBackground();
			DisposeInBackground();
			return result;
		}

		public Slinq<Grouping<K, T, LinkedContext<T>>, GroupByContext<K, T>> SlinqAndKeep()
		{
			return GroupByContext<K, T>.Slinq(this, release: false);
		}

		public Slinq<Grouping<K, T, LinkedContext<T>>, GroupByContext<K, T>> SlinqAndDispose()
		{
			return GroupByContext<K, T>.Slinq(this, release: true);
		}

		public Slinq<Grouping<K, T>, GroupByContext<K, T>> SlinqLinkedAndKeep()
		{
			return GroupByContext<K, T>.SlinqLinked(this, release: false);
		}

		public Slinq<Grouping<K, T>, GroupByContext<K, T>> SlinqLinkedAndDispose()
		{
			return GroupByContext<K, T>.SlinqLinked(this, release: true);
		}

		public Slinq<U, GroupJoinContext<U, K, T, T2, C2>> GroupJoinAndKeep<U, T2, C2>(Slinq<T2, C2> outer, DelegateFunc<T2, K> outerSelector, DelegateFunc<T2, Slinq<T, LinkedContext<T>>, U> resultSelector)
		{
			return GroupJoinContext<U, K, T, T2, C2>.GroupJoin(this, outer, outerSelector, resultSelector, release: false);
		}

		public Slinq<U, GroupJoinContext<U, K, T, T2, C2, P>> GroupJoinAndKeep<U, T2, C2, P>(Slinq<T2, C2> outer, DelegateFunc<T2, P, K> outerSelector, DelegateFunc<T2, Slinq<T, LinkedContext<T>>, P, U> resultSelector, P parameter)
		{
			return GroupJoinContext<U, K, T, T2, C2, P>.GroupJoin(this, outer, outerSelector, resultSelector, parameter, release: false);
		}

		public Slinq<U, GroupJoinContext<U, K, T, T2, C2>> GroupJoinAndDispose<U, T2, C2>(Slinq<T2, C2> outer, DelegateFunc<T2, K> outerSelector, DelegateFunc<T2, Slinq<T, LinkedContext<T>>, U> resultSelector)
		{
			return GroupJoinContext<U, K, T, T2, C2>.GroupJoin(this, outer, outerSelector, resultSelector, release: true);
		}

		public Slinq<U, GroupJoinContext<U, K, T, T2, C2, P>> GroupJoinAndDispose<U, T2, C2, P>(Slinq<T2, C2> outer, DelegateFunc<T2, P, K> outerSelector, DelegateFunc<T2, Slinq<T, LinkedContext<T>>, P, U> resultSelector, P parameter)
		{
			return GroupJoinContext<U, K, T, T2, C2, P>.GroupJoin(this, outer, outerSelector, resultSelector, parameter, release: true);
		}

		public Slinq<U, JoinContext<U, K, T, T2, C2>> JoinAndKeep<U, T2, C2>(Slinq<T2, C2> outer, DelegateFunc<T2, K> outerSelector, DelegateFunc<T2, T, U> resultSelector)
		{
			return JoinContext<U, K, T, T2, C2>.Join(this, outer, outerSelector, resultSelector, release: false);
		}

		public Slinq<U, JoinContext<U, K, T, T2, C2, P>> JoinAndKeep<U, T2, C2, P>(Slinq<T2, C2> outer, DelegateFunc<T2, P, K> outerSelector, DelegateFunc<T2, T, P, U> resultSelector, P parameter)
		{
			return JoinContext<U, K, T, T2, C2, P>.Join(this, outer, outerSelector, resultSelector, parameter, release: false);
		}

		public Slinq<U, JoinContext<U, K, T, T2, C2>> JoinAndDispose<U, T2, C2>(Slinq<T2, C2> outer, DelegateFunc<T2, K> outerSelector, DelegateFunc<T2, T, U> resultSelector)
		{
			return JoinContext<U, K, T, T2, C2>.Join(this, outer, outerSelector, resultSelector, release: true);
		}

		public Slinq<U, JoinContext<U, K, T, T2, C2, P>> JoinAndDispose<U, T2, C2, P>(Slinq<T2, C2> outer, DelegateFunc<T2, P, K> outerSelector, DelegateFunc<T2, T, P, U> resultSelector, P parameter)
		{
			return JoinContext<U, K, T, T2, C2, P>.Join(this, outer, outerSelector, resultSelector, parameter, release: true);
		}
	}
}
