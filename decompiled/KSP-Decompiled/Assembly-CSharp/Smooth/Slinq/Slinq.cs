using System;
using System.Collections.Generic;
using Smooth.Algebraics;
using Smooth.Collections;
using Smooth.Comparisons;
using Smooth.Delegates;
using Smooth.Dispose;
using Smooth.Pools;
using Smooth.Slinq.Collections;
using Smooth.Slinq.Context;

namespace Smooth.Slinq
{
	public struct Slinq<T, C>
	{
		public readonly Mutator<T, C> skip;

		public readonly Mutator<T, C> remove;

		public readonly Mutator<T, C> dispose;

		public C context;

		public Option<T> current;

		public Slinq(Mutator<T, C> skip, Mutator<T, C> remove, Mutator<T, C> dispose, C context)
		{
			this.skip = skip;
			this.remove = remove;
			this.dispose = dispose;
			this.context = context;
			skip(ref this.context, out current);
		}

		public void Skip()
		{
			if (current.isSome)
			{
				skip(ref context, out current);
			}
		}

		public Slinq<T, C> SkipAndReturn()
		{
			if (current.isSome)
			{
				skip(ref context, out current);
			}
			return this;
		}

		public void Remove()
		{
			if (current.isSome)
			{
				remove(ref context, out current);
			}
		}

		public Slinq<T, C> RemoveAndReturn()
		{
			if (current.isSome)
			{
				remove(ref context, out current);
			}
			return this;
		}

		public void Dispose()
		{
			if (current.isSome)
			{
				dispose(ref context, out current);
			}
		}

		public void SkipAll()
		{
			while (current.isSome)
			{
				skip(ref context, out current);
			}
		}

		public Slinq<T, C> Skip(int count)
		{
			while (current.isSome && count-- > 0)
			{
				skip(ref context, out current);
			}
			return this;
		}

		public Slinq<T, C> SkipWhile(DelegateFunc<T, bool> predicate)
		{
			while (current.isSome && predicate(current.value))
			{
				skip(ref context, out current);
			}
			return this;
		}

		public Slinq<T, C> SkipWhile<P>(DelegateFunc<T, P, bool> predicate, P parameter)
		{
			while (current.isSome && predicate(current.value, parameter))
			{
				skip(ref context, out current);
			}
			return this;
		}

		public U SkipWhile<U>(U seed, DelegateFunc<U, T, Option<U>> selector)
		{
			while (current.isSome)
			{
				Option<U> option = selector(seed, current.value);
				if (!option.isSome)
				{
					break;
				}
				skip(ref context, out current);
				seed = option.value;
			}
			return seed;
		}

		public U SkipWhile<U, P>(U seed, DelegateFunc<U, T, P, Option<U>> selector, P parameter)
		{
			while (current.isSome)
			{
				Option<U> option = selector(seed, current.value, parameter);
				if (!option.isSome)
				{
					break;
				}
				skip(ref context, out current);
				seed = option.value;
			}
			return seed;
		}

		public int RemoveAll()
		{
			int num = 0;
			while (current.isSome)
			{
				remove(ref context, out current);
				num++;
			}
			return num;
		}

		public int RemoveAll(DelegateAction<T> then)
		{
			int num = 0;
			while (current.isSome)
			{
				T value = current.value;
				remove(ref context, out current);
				then(value);
				num++;
			}
			return num;
		}

		public int RemoveAll<P>(DelegateAction<T, P> then, P thenParameter)
		{
			int num = 0;
			while (current.isSome)
			{
				T value = current.value;
				remove(ref context, out current);
				then(value, thenParameter);
				num++;
			}
			return num;
		}

		public Slinq<T, C> Remove(int count)
		{
			while (current.isSome && count-- > 0)
			{
				remove(ref context, out current);
			}
			return this;
		}

		public Slinq<T, C> Remove(int count, DelegateAction<T> then)
		{
			while (current.isSome && count-- > 0)
			{
				T value = current.value;
				remove(ref context, out current);
				then(value);
			}
			return this;
		}

		public Slinq<T, C> Remove<P>(int count, DelegateAction<T, P> then, P thenParameter)
		{
			while (current.isSome && count-- > 0)
			{
				T value = current.value;
				remove(ref context, out current);
				then(value, thenParameter);
			}
			return this;
		}

		public Slinq<T, C> RemoveWhile(DelegateFunc<T, bool> predicate)
		{
			while (current.isSome && predicate(current.value))
			{
				remove(ref context, out current);
			}
			return this;
		}

		public Slinq<T, C> RemoveWhile(DelegateFunc<T, bool> predicate, DelegateAction<T> then)
		{
			while (current.isSome && predicate(current.value))
			{
				T value = current.value;
				remove(ref context, out current);
				then(value);
			}
			return this;
		}

		public Slinq<T, C> RemoveWhile<P>(DelegateFunc<T, bool> predicate, DelegateAction<T, P> then, P thenParameter)
		{
			while (current.isSome && predicate(current.value))
			{
				T value = current.value;
				remove(ref context, out current);
				then(value, thenParameter);
			}
			return this;
		}

		public Slinq<T, C> RemoveWhile<P>(DelegateFunc<T, P, bool> predicate, P parameter)
		{
			while (current.isSome && predicate(current.value, parameter))
			{
				remove(ref context, out current);
			}
			return this;
		}

		public Slinq<T, C> RemoveWhile<P>(DelegateFunc<T, P, bool> predicate, P parameter, DelegateAction<T> then)
		{
			while (current.isSome && predicate(current.value, parameter))
			{
				T value = current.value;
				remove(ref context, out current);
				then(value);
			}
			return this;
		}

		public Slinq<T, C> RemoveWhile<P, P2>(DelegateFunc<T, P, bool> predicate, P parameter, DelegateAction<T, P2> then, P2 thenParameter)
		{
			while (current.isSome && predicate(current.value, parameter))
			{
				T value = current.value;
				remove(ref context, out current);
				then(value, thenParameter);
			}
			return this;
		}

		public U RemoveWhile<U>(U seed, DelegateFunc<U, T, Option<U>> selector)
		{
			while (current.isSome)
			{
				Option<U> option = selector(seed, current.value);
				if (!option.isSome)
				{
					break;
				}
				remove(ref context, out current);
				seed = option.value;
			}
			return seed;
		}

		public U RemoveWhile<U>(U seed, DelegateFunc<U, T, Option<U>> selector, DelegateAction<T> then)
		{
			while (current.isSome)
			{
				Option<U> option = selector(seed, current.value);
				if (!option.isSome)
				{
					break;
				}
				T value = current.value;
				remove(ref context, out current);
				then(value);
				seed = option.value;
			}
			return seed;
		}

		public U RemoveWhile<U, P>(U seed, DelegateFunc<U, T, Option<U>> selector, DelegateAction<T, P> then, P thenParameter)
		{
			while (current.isSome)
			{
				Option<U> option = selector(seed, current.value);
				if (!option.isSome)
				{
					break;
				}
				T value = current.value;
				remove(ref context, out current);
				then(value, thenParameter);
				seed = option.value;
			}
			return seed;
		}

		public U RemoveWhile<U, P>(U seed, DelegateFunc<U, T, P, Option<U>> selector, P parameter)
		{
			while (current.isSome)
			{
				Option<U> option = selector(seed, current.value, parameter);
				if (!option.isSome)
				{
					break;
				}
				remove(ref context, out current);
				seed = option.value;
			}
			return seed;
		}

		public U RemoveWhile<U, P>(U seed, DelegateFunc<U, T, P, Option<U>> selector, P parameter, DelegateAction<T> then)
		{
			while (current.isSome)
			{
				Option<U> option = selector(seed, current.value, parameter);
				if (!option.isSome)
				{
					break;
				}
				T value = current.value;
				remove(ref context, out current);
				then(value);
				seed = option.value;
			}
			return seed;
		}

		public U RemoveWhile<U, P, P2>(U seed, DelegateFunc<U, T, P, Option<U>> selector, P parameter, DelegateAction<T, P2> then, P2 thenParameter)
		{
			while (current.isSome)
			{
				Option<U> option = selector(seed, current.value, parameter);
				if (!option.isSome)
				{
					break;
				}
				T value = current.value;
				remove(ref context, out current);
				then(value, thenParameter);
				seed = option.value;
			}
			return seed;
		}

		public T Aggregate(DelegateFunc<T, T, T> selector)
		{
			return AggregateOrNone(selector).ValueOr(delegate
			{
				throw new InvalidOperationException();
			});
		}

		public Option<T> AggregateOrNone(DelegateFunc<T, T, T> selector)
		{
			if (current.isSome)
			{
				T val = current.value;
				skip(ref context, out current);
				while (current.isSome)
				{
					val = selector(val, current.value);
					skip(ref context, out current);
				}
				return new Option<T>(val);
			}
			return current;
		}

		public Option<T> AggregateOrNone<P>(DelegateFunc<T, T, P, T> selector, P parameter)
		{
			if (current.isSome)
			{
				T val = current.value;
				skip(ref context, out current);
				while (current.isSome)
				{
					val = selector(val, current.value, parameter);
					skip(ref context, out current);
				}
				return new Option<T>(val);
			}
			return current;
		}

		public U Aggregate<U>(U seed, DelegateFunc<U, T, U> selector)
		{
			while (current.isSome)
			{
				seed = selector(seed, current.value);
				skip(ref context, out current);
			}
			return seed;
		}

		public V Aggregate<U, V>(U seed, DelegateFunc<U, T, U> selector, DelegateFunc<U, V> resultSelector)
		{
			return resultSelector(Aggregate(seed, selector));
		}

		public U Aggregate<U, P>(U seed, DelegateFunc<U, T, P, U> selector, P parameter)
		{
			while (current.isSome)
			{
				seed = selector(seed, current.value, parameter);
				skip(ref context, out current);
			}
			return seed;
		}

		public U AggregateWhile<U>(U seed, DelegateFunc<U, T, Option<U>> selector)
		{
			while (current.isSome)
			{
				Option<U> option = selector(seed, current.value);
				if (option.isSome)
				{
					skip(ref context, out current);
					seed = option.value;
					continue;
				}
				dispose(ref context, out current);
				break;
			}
			return seed;
		}

		public U AggregateWhile<U, P>(U seed, DelegateFunc<U, T, P, Option<U>> selector, P parameter)
		{
			while (current.isSome)
			{
				Option<U> option = selector(seed, current.value, parameter);
				if (option.isSome)
				{
					skip(ref context, out current);
					seed = option.value;
					continue;
				}
				dispose(ref context, out current);
				break;
			}
			return seed;
		}

		public bool All(DelegateFunc<T, bool> predicate)
		{
			while (true)
			{
				if (current.isSome)
				{
					if (!predicate(current.value))
					{
						break;
					}
					skip(ref context, out current);
					continue;
				}
				return true;
			}
			dispose(ref context, out current);
			return false;
		}

		public bool All<P>(DelegateFunc<T, P, bool> predicate, P parameter)
		{
			while (true)
			{
				if (current.isSome)
				{
					if (!predicate(current.value, parameter))
					{
						break;
					}
					skip(ref context, out current);
					continue;
				}
				return true;
			}
			dispose(ref context, out current);
			return false;
		}

		public bool Any()
		{
			if (current.isSome)
			{
				dispose(ref context, out current);
				return true;
			}
			return false;
		}

		public bool Any(DelegateFunc<T, bool> predicate)
		{
			while (true)
			{
				if (current.isSome)
				{
					if (predicate(current.value))
					{
						break;
					}
					skip(ref context, out current);
					continue;
				}
				return false;
			}
			dispose(ref context, out current);
			return true;
		}

		public bool Any<P>(DelegateFunc<T, P, bool> predicate, P parameter)
		{
			while (true)
			{
				if (current.isSome)
				{
					if (predicate(current.value, parameter))
					{
						break;
					}
					skip(ref context, out current);
					continue;
				}
				return false;
			}
			dispose(ref context, out current);
			return true;
		}

		public bool Contains(T value)
		{
			return Contains(value, Smooth.Collections.EqualityComparer<T>.Default);
		}

		public bool Contains(T value, IEqualityComparer<T> comparer)
		{
			while (true)
			{
				if (current.isSome)
				{
					if (comparer.Equals(value, current.value))
					{
						break;
					}
					skip(ref context, out current);
					continue;
				}
				return false;
			}
			dispose(ref context, out current);
			return true;
		}

		public int Count()
		{
			int num = 0;
			while (current.isSome)
			{
				num++;
				skip(ref context, out current);
			}
			return num;
		}

		public T ElementAt(int index)
		{
			return ElementAtOrNone(index).ValueOr(delegate
			{
				throw new ArgumentOutOfRangeException();
			});
		}

		public T ElementAtOrDefault(int index)
		{
			return ElementAtOrNone(index).value;
		}

		public Option<T> ElementAtOrNone(int index)
		{
			if (index > 0)
			{
				return Skip(index - 1).FirstOrNone();
			}
			if (index == 0)
			{
				return FirstOrNone();
			}
			Dispose();
			return Option<T>.None;
		}

		public T First()
		{
			return FirstOrNone().ValueOr(delegate
			{
				throw new InvalidOperationException();
			});
		}

		public T First(DelegateFunc<T, bool> predicate)
		{
			return FirstOrNone(predicate).ValueOr(delegate
			{
				throw new InvalidOperationException();
			});
		}

		public T FirstOrDefault()
		{
			return FirstOrNone().value;
		}

		public T FirstOrDefault(DelegateFunc<T, bool> predicate)
		{
			return FirstOrNone(predicate).value;
		}

		public Option<T> FirstOrNone()
		{
			if (current.isSome)
			{
				Option<T> result = current;
				dispose(ref context, out current);
				return result;
			}
			return current;
		}

		public Option<T> FirstOrNone(DelegateFunc<T, bool> predicate)
		{
			while (current.isSome && !predicate(current.value))
			{
				skip(ref context, out current);
			}
			return FirstOrNone();
		}

		public Option<T> FirstOrNone<P>(DelegateFunc<T, P, bool> predicate, P parameter)
		{
			while (current.isSome && !predicate(current.value, parameter))
			{
				skip(ref context, out current);
			}
			return FirstOrNone();
		}

		public void ForEach(DelegateAction<T> action)
		{
			while (current.isSome)
			{
				action(current.value);
				skip(ref context, out current);
			}
		}

		public void ForEach<P>(DelegateAction<T, P> action, P parameter)
		{
			while (current.isSome)
			{
				action(current.value, parameter);
				skip(ref context, out current);
			}
		}

		public T Last()
		{
			return LastOrNone().ValueOr(delegate
			{
				throw new InvalidOperationException();
			});
		}

		public T Last(DelegateFunc<T, bool> predicate)
		{
			return LastOrNone(predicate).ValueOr(delegate
			{
				throw new InvalidOperationException();
			});
		}

		public T LastOrDefault()
		{
			return LastOrNone().value;
		}

		public T LastOrDefault(DelegateFunc<T, bool> predicate)
		{
			return LastOrNone(predicate).value;
		}

		public Option<T> LastOrNone()
		{
			Option<T> none = Option<T>.None;
			while (current.isSome)
			{
				none = current;
				skip(ref context, out current);
			}
			return none;
		}

		public Option<T> LastOrNone(DelegateFunc<T, bool> predicate)
		{
			Option<T> none = Option<T>.None;
			while (current.isSome)
			{
				if (predicate(current.value))
				{
					none = current;
				}
				skip(ref context, out current);
			}
			return none;
		}

		public Option<T> LastOrNone<P>(DelegateFunc<T, P, bool> predicate, P parameter)
		{
			Option<T> none = Option<T>.None;
			while (current.isSome)
			{
				if (predicate(current.value, parameter))
				{
					none = current;
				}
				skip(ref context, out current);
			}
			return none;
		}

		public T Max()
		{
			return MaxOrNone().ValueOr(delegate
			{
				if (!typeof(T).IsClass)
				{
					throw new InvalidOperationException();
				}
				return default(T);
			});
		}

		public Option<T> MaxOrNone()
		{
			return MaxOrNone(Comparisons<T>.Default);
		}

		public Option<T> MaxOrNone(IComparer<T> comparer)
		{
			return MaxOrNone(Comparisons<T>.ToComparison(comparer));
		}

		public Option<T> MaxOrNone(Comparison<T> comparison)
		{
			if (current.isSome)
			{
				T value = current.value;
				skip(ref context, out current);
				while (current.isSome)
				{
					if (comparison(value, current.value) < 0)
					{
						value = current.value;
					}
					skip(ref context, out current);
				}
				return new Option<T>(value);
			}
			return current;
		}

		public Option<T> MaxOrNone<K>(DelegateFunc<T, K> selector)
		{
			return MaxOrNone(selector, Comparisons<K>.Default);
		}

		public Option<T> MaxOrNone<K>(DelegateFunc<T, K> selector, IComparer<K> comparer)
		{
			return MaxOrNone(selector, Comparisons<K>.ToComparison(comparer));
		}

		public Option<T> MaxOrNone<K>(DelegateFunc<T, K> selector, Comparison<K> comparison)
		{
			if (current.isSome)
			{
				K x = selector(current.value);
				T value = current.value;
				skip(ref context, out current);
				while (current.isSome)
				{
					K val = selector(current.value);
					if (comparison(x, val) < 0)
					{
						x = val;
						value = current.value;
					}
					skip(ref context, out current);
				}
				return new Option<T>(value);
			}
			return current;
		}

		public Option<T> MaxOrNone<K, P>(DelegateFunc<T, P, K> selector, P parameter)
		{
			return MaxOrNone(selector, parameter, Comparisons<K>.Default);
		}

		public Option<T> MaxOrNone<K, P>(DelegateFunc<T, P, K> selector, P parameter, IComparer<K> comparer)
		{
			return MaxOrNone(selector, parameter, Comparisons<K>.ToComparison(comparer));
		}

		public Option<T> MaxOrNone<K, P>(DelegateFunc<T, P, K> selector, P parameter, Comparison<K> comparison)
		{
			if (current.isSome)
			{
				K x = selector(current.value, parameter);
				T value = current.value;
				skip(ref context, out current);
				while (current.isSome)
				{
					K val = selector(current.value, parameter);
					if (comparison(x, val) < 0)
					{
						x = val;
						value = current.value;
					}
					skip(ref context, out current);
				}
				return new Option<T>(value);
			}
			return current;
		}

		public T Min()
		{
			return MinOrNone().ValueOr(delegate
			{
				if (!typeof(T).IsClass)
				{
					throw new InvalidOperationException();
				}
				return default(T);
			});
		}

		public Option<T> MinOrNone()
		{
			return MinOrNone(Comparisons<T>.Default);
		}

		public Option<T> Min(IComparer<T> comparer)
		{
			return MinOrNone(Comparisons<T>.ToComparison(comparer));
		}

		public Option<T> MinOrNone(Comparison<T> comparison)
		{
			if (current.isSome)
			{
				T value = current.value;
				skip(ref context, out current);
				while (current.isSome)
				{
					if (comparison(value, current.value) > 0)
					{
						value = current.value;
					}
					skip(ref context, out current);
				}
				return new Option<T>(value);
			}
			return current;
		}

		public Option<T> MinOrNone<K>(DelegateFunc<T, K> selector)
		{
			return MinOrNone(selector, Comparisons<K>.Default);
		}

		public Option<T> MinOrNone<K>(DelegateFunc<T, K> selector, IComparer<K> comparer)
		{
			return MinOrNone(selector, Comparisons<K>.ToComparison(comparer));
		}

		public Option<T> MinOrNone<K>(DelegateFunc<T, K> selector, Comparison<K> comparison)
		{
			if (current.isSome)
			{
				K x = selector(current.value);
				T value = current.value;
				skip(ref context, out current);
				while (current.isSome)
				{
					K val = selector(current.value);
					if (comparison(x, val) > 0)
					{
						x = val;
						value = current.value;
					}
					skip(ref context, out current);
				}
				return new Option<T>(value);
			}
			return current;
		}

		public Option<T> MinOrNone<K, P>(DelegateFunc<T, P, K> selector, P parameter)
		{
			return MinOrNone(selector, parameter, Comparisons<K>.Default);
		}

		public Option<T> MinOrNone<K, P>(DelegateFunc<T, P, K> selector, P parameter, IComparer<K> comparer)
		{
			return MinOrNone(selector, parameter, Comparisons<K>.ToComparison(comparer));
		}

		public Option<T> MinOrNone<K, P>(DelegateFunc<T, P, K> selector, P parameter, Comparison<K> comparison)
		{
			if (current.isSome)
			{
				K x = selector(current.value, parameter);
				T value = current.value;
				skip(ref context, out current);
				while (current.isSome)
				{
					K val = selector(current.value, parameter);
					if (comparison(x, val) > 0)
					{
						x = val;
						value = current.value;
					}
					skip(ref context, out current);
				}
				return new Option<T>(value);
			}
			return current;
		}

		public bool SequenceEqual<C2>(Slinq<T, C2> other)
		{
			return SequenceEqual(other, Comparisons<T>.ToPredicate(Smooth.Collections.EqualityComparer<T>.Default));
		}

		public bool SequenceEqual<C2>(Slinq<T, C2> other, System.Collections.Generic.EqualityComparer<T> equalityComparer)
		{
			return SequenceEqual(other, Comparisons<T>.ToPredicate(equalityComparer));
		}

		public bool SequenceEqual<T2, C2>(Slinq<T2, C2> other, DelegateFunc<T, T2, bool> predicate)
		{
			while (current.isSome && other.current.isSome)
			{
				if (predicate(current.value, other.current.value))
				{
					skip(ref context, out current);
					other.skip(ref other.context, out other.current);
					continue;
				}
				dispose(ref context, out current);
				other.dispose(ref other.context, out other.current);
				return false;
			}
			if (current.isSome)
			{
				dispose(ref context, out current);
				return false;
			}
			if (other.current.isSome)
			{
				other.dispose(ref other.context, out other.current);
				return false;
			}
			return true;
		}

		public T Single()
		{
			return SingleOrNone().ValueOr(delegate
			{
				throw new InvalidOperationException();
			});
		}

		public T SingleOrDefault()
		{
			if (current.isSome)
			{
				return SingleOrNone().ValueOr(delegate
				{
					throw new InvalidOperationException();
				});
			}
			return default(T);
		}

		public Option<T> SingleOrNone()
		{
			if (current.isSome)
			{
				Option<T> result = current;
				skip(ref context, out current);
				if (current.isSome)
				{
					dispose(ref context, out current);
					return default(Option<T>);
				}
				return result;
			}
			return current;
		}

		public Smooth.Algebraics.Tuple<LinkedHeadTail<T>, LinkedHeadTail<T>> SplitRight(int count)
		{
			if (current.isSome)
			{
				if (count > 0)
				{
					LinkedHeadTail<T> item = default(LinkedHeadTail<T>);
					LinkedHeadTail<T> item2 = new LinkedHeadTail<T>(current.value);
					skip(ref context, out current);
					while (current.isSome && item2.count <= count)
					{
						item2.tail.next = Linked<T>.Borrow(current.value);
						item2.tail = item2.tail.next;
						item2.count++;
						skip(ref context, out current);
					}
					if (item2.count > count)
					{
						item.head = item2.head;
						item.tail = item2.head;
						item.count = 1;
						item2.head = item2.head.next;
						item2.count--;
						while (current.isSome)
						{
							item2.tail.next = Linked<T>.Borrow(current.value);
							item2.tail = item2.tail.next;
							item.tail = item2.head;
							item2.head = item2.head.next;
							item.count++;
							skip(ref context, out current);
						}
						item.tail.next = null;
					}
					return new Smooth.Algebraics.Tuple<LinkedHeadTail<T>, LinkedHeadTail<T>>(item, item2);
				}
				return new Smooth.Algebraics.Tuple<LinkedHeadTail<T>, LinkedHeadTail<T>>(ToLinked(), default(LinkedHeadTail<T>));
			}
			return default(Smooth.Algebraics.Tuple<LinkedHeadTail<T>, LinkedHeadTail<T>>);
		}

		public IC AddTo<IC>(IC collection) where IC : ICollection<T>
		{
			while (current.isSome)
			{
				collection.Add(current.value);
				skip(ref context, out current);
			}
			return collection;
		}

		public Disposable<IC> AddTo<IC>(Disposable<IC> collection) where IC : ICollection<T>
		{
			AddTo(collection.value);
			return collection;
		}

		public IC AddTo<U, IC>(IC collection, DelegateFunc<T, U> selector) where IC : ICollection<U>
		{
			while (current.isSome)
			{
				collection.Add(selector(current.value));
				skip(ref context, out current);
			}
			return collection;
		}

		public Disposable<IC> AddTo<U, IC>(Disposable<IC> collection, DelegateFunc<T, U> selector) where IC : ICollection<U>
		{
			AddTo(collection.value, selector);
			return collection;
		}

		public IC AddTo<U, IC, P>(IC collection, DelegateFunc<T, P, U> selector, P parameter) where IC : ICollection<U>
		{
			while (current.isSome)
			{
				collection.Add(selector(current.value, parameter));
				skip(ref context, out current);
			}
			return collection;
		}

		public Disposable<IC> AddTo<U, IC, P>(Disposable<IC> collection, DelegateFunc<T, P, U> selector, P parameter) where IC : ICollection<U>
		{
			AddTo(collection.value, selector, parameter);
			return collection;
		}

		public LinkedHeadTail<T> AddTo(LinkedHeadTail<T> list)
		{
			if (current.isSome)
			{
				list.Append(current.value);
				skip(ref context, out current);
				while (current.isSome)
				{
					list.tail.next = Linked<T>.Borrow(current.value);
					list.tail = list.tail.next;
					list.count++;
					skip(ref context, out current);
				}
			}
			return list;
		}

		public LinkedHeadTail<T> AddToReverse(LinkedHeadTail<T> list)
		{
			if (current.isSome)
			{
				LinkedHeadTail<T> other = new LinkedHeadTail<T>(current.value);
				skip(ref context, out current);
				while (current.isSome)
				{
					Linked<T> linked = Linked<T>.Borrow(current.value);
					linked.next = other.head;
					other.head = linked;
					other.count++;
					skip(ref context, out current);
				}
				list.Append(other);
				return list;
			}
			return list;
		}

		public LinkedHeadTail<K, T> AddTo<K>(LinkedHeadTail<K, T> list, DelegateFunc<T, K> selector)
		{
			if (current.isSome)
			{
				list.Append(selector(current.value), current.value);
				skip(ref context, out current);
				while (current.isSome)
				{
					list.tail.next = Linked<K, T>.Borrow(selector(current.value), current.value);
					list.tail = list.tail.next;
					list.count++;
					skip(ref context, out current);
				}
			}
			return list;
		}

		public LinkedHeadTail<K, T> AddTo<K, P>(LinkedHeadTail<K, T> list, DelegateFunc<T, P, K> selector, P parameter)
		{
			if (current.isSome)
			{
				list.Append(selector(current.value, parameter), current.value);
				skip(ref context, out current);
				while (current.isSome)
				{
					list.tail.next = Linked<K, T>.Borrow(selector(current.value, parameter), current.value);
					list.tail = list.tail.next;
					list.count++;
					skip(ref context, out current);
				}
			}
			return list;
		}

		public LinkedHeadTail<K, T> AddToReverse<K>(LinkedHeadTail<K, T> list, DelegateFunc<T, K> selector)
		{
			if (current.isSome)
			{
				LinkedHeadTail<K, T> other = new LinkedHeadTail<K, T>(selector(current.value), current.value);
				skip(ref context, out current);
				while (current.isSome)
				{
					Linked<K, T> linked = Linked<K, T>.Borrow(selector(current.value), current.value);
					linked.next = other.head;
					other.head = linked;
					other.count++;
					skip(ref context, out current);
				}
				list.Append(other);
				return list;
			}
			return list;
		}

		public LinkedHeadTail<K, T> AddToReverse<K, P>(LinkedHeadTail<K, T> list, DelegateFunc<T, P, K> selector, P parameter)
		{
			if (current.isSome)
			{
				LinkedHeadTail<K, T> other = new LinkedHeadTail<K, T>(selector(current.value, parameter), current.value);
				skip(ref context, out current);
				while (current.isSome)
				{
					Linked<K, T> linked = Linked<K, T>.Borrow(selector(current.value, parameter), current.value);
					linked.next = other.head;
					other.head = linked;
					other.count++;
					skip(ref context, out current);
				}
				list.Append(other);
				return list;
			}
			return list;
		}

		public Lookup<K, T> AddTo<K>(Lookup<K, T> lookup, DelegateFunc<T, K> selector)
		{
			while (current.isSome)
			{
				lookup.Add(selector(current.value), current.value);
				skip(ref context, out current);
			}
			return lookup;
		}

		public Lookup<K, T> AddTo<K, P>(Lookup<K, T> lookup, DelegateFunc<T, P, K> selector, P parameter)
		{
			while (current.isSome)
			{
				lookup.Add(selector(current.value, parameter), current.value);
				skip(ref context, out current);
			}
			return lookup;
		}

		public LinkedHeadTail<T> ToLinked()
		{
			return AddTo(default(LinkedHeadTail<T>));
		}

		public LinkedHeadTail<T> ToLinkedReverse()
		{
			return AddToReverse(default(LinkedHeadTail<T>));
		}

		public LinkedHeadTail<K, T> ToLinked<K>(DelegateFunc<T, K> selector)
		{
			return AddTo(default(LinkedHeadTail<K, T>), selector);
		}

		public LinkedHeadTail<K, T> ToLinked<K, P>(DelegateFunc<T, P, K> selector, P parameter)
		{
			return AddTo(default(LinkedHeadTail<K, T>), selector, parameter);
		}

		public LinkedHeadTail<K, T> ToLinkedReverse<K>(DelegateFunc<T, K> selector)
		{
			return AddToReverse(default(LinkedHeadTail<K, T>), selector);
		}

		public LinkedHeadTail<K, T> ToLinkedReverse<K, P>(DelegateFunc<T, P, K> selector, P parameter)
		{
			return AddToReverse(default(LinkedHeadTail<K, T>), selector, parameter);
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

		public List<T> ToList()
		{
			return AddTo(ListPool<T>.Instance.Borrow());
		}
	}
	public static class Slinq
	{
		public static Slinq<U, AggregateContext<U, T, C>> AggregateRunning<U, T, C>(this Slinq<T, C> slinq, U seed, DelegateFunc<U, T, U> selector)
		{
			return AggregateContext<U, T, C>.AggregateRunning(slinq, seed, selector);
		}

		public static Slinq<U, AggregateContext<U, T, C, P>> AggregateRunning<U, T, C, P>(this Slinq<T, C> slinq, U seed, DelegateFunc<U, T, P, U> selector, P parameter)
		{
			return AggregateContext<U, T, C, P>.AggregateRunning(slinq, seed, selector, parameter);
		}

		public static Slinq<T, ConcatContext<C2, T, C>> Concat<C2, T, C>(this Slinq<T, C> first, Slinq<T, C2> second)
		{
			return ConcatContext<C2, T, C>.Concat(first, second);
		}

		public static Slinq<T, EitherContext<OptionContext<T>, T, C>> DefaultIfEmpty<T, C>(this Slinq<T, C> slinq)
		{
			if (!slinq.current.isSome)
			{
				return EitherContext<OptionContext<T>, T, C>.Right(OptionContext<T>.Slinq(new Option<T>(default(T))));
			}
			return EitherContext<OptionContext<T>, T, C>.Left(slinq);
		}

		public static Slinq<T, EitherContext<OptionContext<T>, T, C>> DefaultIfEmpty<T, C>(this Slinq<T, C> slinq, T defaultValue)
		{
			if (!slinq.current.isSome)
			{
				return EitherContext<OptionContext<T>, T, C>.Right(OptionContext<T>.Slinq(new Option<T>(defaultValue)));
			}
			return EitherContext<OptionContext<T>, T, C>.Left(slinq);
		}

		public static Slinq<T, HashSetContext<T, C>> Distinct<T, C>(this Slinq<T, C> slinq)
		{
			return HashSetContext<T, C>.Distinct(slinq, HashSetPool<T>.Instance.BorrowDisposable(), release: true);
		}

		public static Slinq<T, HashSetContext<T, C>> Distinct<T, C>(this Slinq<T, C> slinq, IEqualityComparer<T> comparer)
		{
			return HashSetContext<T, C>.Distinct(slinq, HashSetPool<T>.Instance.BorrowDisposable(comparer), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C>> Distinct<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector)
		{
			return HashSetContext<K, T, C>.Distinct(slinq, selector, HashSetPool<K>.Instance.BorrowDisposable(), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C>> Distinct<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector, IEqualityComparer<K> comparer)
		{
			return HashSetContext<K, T, C>.Distinct(slinq, selector, HashSetPool<K>.Instance.BorrowDisposable(comparer), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C, P>> Distinct<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter)
		{
			return HashSetContext<K, T, C, P>.Distinct(slinq, selector, parameter, HashSetPool<K>.Instance.BorrowDisposable(), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C, P>> Distinct<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter, IEqualityComparer<K> comparer)
		{
			return HashSetContext<K, T, C, P>.Distinct(slinq, selector, parameter, HashSetPool<K>.Instance.BorrowDisposable(comparer), release: true);
		}

		public static Slinq<T, HashSetContext<T, C>> Except<C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other)
		{
			return HashSetContext<T, C>.Except(slinq, other.AddTo(HashSetPool<T>.Instance.BorrowDisposable()), release: true);
		}

		public static Slinq<T, HashSetContext<T, C>> Except<C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other, IEqualityComparer<T> comparer)
		{
			return HashSetContext<T, C>.Except(slinq, other.AddTo(HashSetPool<T>.Instance.BorrowDisposable(comparer)), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C>> Except<K, C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, K> selector)
		{
			return HashSetContext<K, T, C>.Except(slinq, selector, other.AddTo(HashSetPool<K>.Instance.BorrowDisposable(), selector), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C>> Except<K, C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, K> selector, IEqualityComparer<K> comparer)
		{
			return HashSetContext<K, T, C>.Except(slinq, selector, other.AddTo(HashSetPool<K>.Instance.BorrowDisposable(comparer), selector), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C, P>> Except<K, C2, T, C, P>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, P, K> selector, P parameter)
		{
			return HashSetContext<K, T, C, P>.Except(slinq, selector, parameter, other.AddTo(HashSetPool<K>.Instance.BorrowDisposable(), selector, parameter), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C, P>> Except<K, C2, T, C, P>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, P, K> selector, P parameter, IEqualityComparer<K> comparer)
		{
			return HashSetContext<K, T, C, P>.Except(slinq, selector, parameter, other.AddTo(HashSetPool<K>.Instance.BorrowDisposable(comparer), selector, parameter), release: true);
		}

		public static Slinq<T, FlattenContext<T, C1, C2>> Flatten<T, C1, C2>(this Slinq<Slinq<T, C1>, C2> slinq)
		{
			return FlattenContext<T, C1, C2>.Flatten(slinq);
		}

		public static Slinq<T, FlattenContext<T, C>> Flatten<T, C>(this Slinq<Option<T>, C> slinq)
		{
			return FlattenContext<T, C>.Flatten(slinq);
		}

		public static Slinq<Grouping<K, T, LinkedContext<T>>, GroupByContext<K, T>> GroupBy<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector)
		{
			return slinq.ToLookup(selector, Smooth.Collections.EqualityComparer<K>.Default).SlinqAndDispose();
		}

		public static Slinq<Grouping<K, T, LinkedContext<T>>, GroupByContext<K, T>> GroupBy<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector, IEqualityComparer<K> comparer)
		{
			return slinq.ToLookup(selector, comparer).SlinqAndDispose();
		}

		public static Slinq<Grouping<K, T, LinkedContext<T>>, GroupByContext<K, T>> GroupBy<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter)
		{
			return slinq.ToLookup(selector, parameter, Smooth.Collections.EqualityComparer<K>.Default).SlinqAndDispose();
		}

		public static Slinq<Grouping<K, T, LinkedContext<T>>, GroupByContext<K, T>> GroupBy<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter, IEqualityComparer<K> comparer)
		{
			return slinq.ToLookup(selector, parameter, comparer).SlinqAndDispose();
		}

		public static Slinq<U, GroupJoinContext<U, K, T2, T, C>> GroupJoin<U, K, T2, C2, T, C>(this Slinq<T, C> outer, Slinq<T2, C2> inner, DelegateFunc<T, K> outerSelector, DelegateFunc<T2, K> innerSelector, DelegateFunc<T, Slinq<T2, LinkedContext<T2>>, U> resultSelector)
		{
			return inner.ToLookup(innerSelector, Smooth.Collections.EqualityComparer<K>.Default).GroupJoinAndDispose(outer, outerSelector, resultSelector);
		}

		public static Slinq<U, GroupJoinContext<U, K, T2, T, C>> GroupJoin<U, K, T2, C2, T, C>(this Slinq<T, C> outer, Slinq<T2, C2> inner, DelegateFunc<T, K> outerSelector, DelegateFunc<T2, K> innerSelector, DelegateFunc<T, Slinq<T2, LinkedContext<T2>>, U> resultSelector, IEqualityComparer<K> comparer)
		{
			return inner.ToLookup(innerSelector, comparer).GroupJoinAndDispose(outer, outerSelector, resultSelector);
		}

		public static Slinq<U, GroupJoinContext<U, K, T2, T, C, P>> GroupJoin<U, K, T2, C2, T, C, P>(this Slinq<T, C> outer, Slinq<T2, C2> inner, DelegateFunc<T, P, K> outerSelector, DelegateFunc<T2, P, K> innerSelector, DelegateFunc<T, Slinq<T2, LinkedContext<T2>>, P, U> resultSelector, P parameter)
		{
			return inner.ToLookup(innerSelector, parameter, Smooth.Collections.EqualityComparer<K>.Default).GroupJoinAndDispose(outer, outerSelector, resultSelector, parameter);
		}

		public static Slinq<U, GroupJoinContext<U, K, T2, T, C, P>> GroupJoin<U, K, T2, C2, T, C, P>(this Slinq<T, C> outer, Slinq<T2, C2> inner, DelegateFunc<T, P, K> outerSelector, DelegateFunc<T2, P, K> innerSelector, DelegateFunc<T, Slinq<T2, LinkedContext<T2>>, P, U> resultSelector, P parameter, IEqualityComparer<K> comparer)
		{
			return inner.ToLookup(innerSelector, parameter, comparer).GroupJoinAndDispose(outer, outerSelector, resultSelector, parameter);
		}

		public static Slinq<T, HashSetContext<T, C>> Intersect<C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other)
		{
			return HashSetContext<T, C>.Intersect(slinq, other.AddTo(HashSetPool<T>.Instance.BorrowDisposable()), release: true);
		}

		public static Slinq<T, HashSetContext<T, C>> Intersect<C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other, IEqualityComparer<T> comparer)
		{
			return HashSetContext<T, C>.Intersect(slinq, other.AddTo(HashSetPool<T>.Instance.BorrowDisposable(comparer)), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C>> Intersect<K, C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, K> selector)
		{
			return HashSetContext<K, T, C>.Intersect(slinq, selector, other.AddTo(HashSetPool<K>.Instance.BorrowDisposable(), selector), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C>> Intersect<K, C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, K> selector, IEqualityComparer<K> comparer)
		{
			return HashSetContext<K, T, C>.Intersect(slinq, selector, other.AddTo(HashSetPool<K>.Instance.BorrowDisposable(comparer), selector), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C, P>> Intersect<K, C2, T, C, P>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, P, K> selector, P parameter)
		{
			return HashSetContext<K, T, C, P>.Intersect(slinq, selector, parameter, other.AddTo(HashSetPool<K>.Instance.BorrowDisposable(), selector, parameter), release: true);
		}

		public static Slinq<T, HashSetContext<K, T, C, P>> Intersect<K, C2, T, C, P>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, P, K> selector, P parameter, IEqualityComparer<K> comparer)
		{
			return HashSetContext<K, T, C, P>.Intersect(slinq, selector, parameter, other.AddTo(HashSetPool<K>.Instance.BorrowDisposable(comparer), selector, parameter), release: true);
		}

		public static Slinq<U, JoinContext<U, K, T2, T, C>> Join<U, K, T2, C2, T, C>(this Slinq<T, C> outer, Slinq<T2, C2> inner, DelegateFunc<T, K> outerSelector, DelegateFunc<T2, K> innerSelector, DelegateFunc<T, T2, U> resultSelector)
		{
			return inner.ToLookup(innerSelector, Smooth.Collections.EqualityComparer<K>.Default).JoinAndDispose(outer, outerSelector, resultSelector);
		}

		public static Slinq<U, JoinContext<U, K, T2, T, C>> Join<U, K, T2, C2, T, C>(this Slinq<T, C> outer, Slinq<T2, C2> inner, DelegateFunc<T, K> outerSelector, DelegateFunc<T2, K> innerSelector, DelegateFunc<T, T2, U> resultSelector, IEqualityComparer<K> comparer)
		{
			return inner.ToLookup(innerSelector, comparer).JoinAndDispose(outer, outerSelector, resultSelector);
		}

		public static Slinq<U, JoinContext<U, K, T2, T, C, P>> Join<U, K, T2, C2, T, C, P>(this Slinq<T, C> outer, Slinq<T2, C2> inner, DelegateFunc<T, P, K> outerSelector, DelegateFunc<T2, P, K> innerSelector, DelegateFunc<T, T2, P, U> resultSelector, P parameter)
		{
			return inner.ToLookup(innerSelector, parameter, Smooth.Collections.EqualityComparer<K>.Default).JoinAndDispose(outer, outerSelector, resultSelector, parameter);
		}

		public static Slinq<U, JoinContext<U, K, T2, T, C, P>> Join<U, K, T2, C2, T, C, P>(this Slinq<T, C> outer, Slinq<T2, C2> inner, DelegateFunc<T, P, K> outerSelector, DelegateFunc<T2, P, K> innerSelector, DelegateFunc<T, T2, P, U> resultSelector, P parameter, IEqualityComparer<K> comparer)
		{
			return inner.ToLookup(innerSelector, parameter, comparer).JoinAndDispose(outer, outerSelector, resultSelector, parameter);
		}

		public static Slinq<T, LinkedContext<T>> OrderBy<T, C>(this Slinq<T, C> slinq)
		{
			return slinq.OrderBy(Comparisons<T>.ToComparison(Smooth.Collections.Comparer<T>.Default), ascending: true);
		}

		public static Slinq<T, LinkedContext<T>> OrderBy<T, C>(this Slinq<T, C> slinq, IComparer<T> comparer)
		{
			return slinq.OrderBy(Comparisons<T>.ToComparison(comparer), ascending: true);
		}

		public static Slinq<T, LinkedContext<T>> OrderBy<T, C>(this Slinq<T, C> slinq, Comparison<T> comparison)
		{
			return slinq.OrderBy(comparison, ascending: true);
		}

		public static Slinq<T, LinkedContext<T>> OrderByDescending<T, C>(this Slinq<T, C> slinq)
		{
			return slinq.OrderBy(Comparisons<T>.ToComparison(Smooth.Collections.Comparer<T>.Default), ascending: false);
		}

		public static Slinq<T, LinkedContext<T>> OrderByDescending<T, C>(this Slinq<T, C> slinq, IComparer<T> comparer)
		{
			return slinq.OrderBy(Comparisons<T>.ToComparison(comparer), ascending: false);
		}

		public static Slinq<T, LinkedContext<T>> OrderByDescending<T, C>(this Slinq<T, C> slinq, Comparison<T> comparison)
		{
			return slinq.OrderBy(comparison, ascending: false);
		}

		public static Slinq<T, LinkedContext<T>> OrderBy<T, C>(this Slinq<T, C> slinq, Comparison<T> comparison, bool ascending)
		{
			if (!slinq.current.isSome)
			{
				return default(Slinq<T, LinkedContext<T>>);
			}
			return Linked.Sort(slinq.ToLinked(), comparison, ascending).SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<K, T>> OrderBy<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector)
		{
			return slinq.OrderBy(selector, Comparisons<K>.ToComparison(Smooth.Collections.Comparer<K>.Default), ascending: true);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderBy<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector, IComparer<K> comparer)
		{
			return slinq.OrderBy(selector, Comparisons<K>.ToComparison(comparer), ascending: true);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderBy<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector, Comparison<K> comparison)
		{
			return slinq.OrderBy(selector, comparison, ascending: true);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderByDescending<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector)
		{
			return slinq.OrderBy(selector, Comparisons<K>.ToComparison(Smooth.Collections.Comparer<K>.Default), ascending: false);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderByDescending<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector, IComparer<K> comparer)
		{
			return slinq.OrderBy(selector, Comparisons<K>.ToComparison(comparer), ascending: false);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderByDescending<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector, Comparison<K> comparison)
		{
			return slinq.OrderBy(selector, comparison, ascending: false);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderBy<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector, Comparison<K> comparison, bool ascending)
		{
			if (!slinq.current.isSome)
			{
				return default(Slinq<T, LinkedContext<K, T>>);
			}
			return Linked.Sort(slinq.ToLinked(selector), comparison, ascending).SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<K, T>> OrderBy<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter)
		{
			return slinq.OrderBy(selector, parameter, Comparisons<K>.ToComparison(Smooth.Collections.Comparer<K>.Default), ascending: true);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderBy<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter, IComparer<K> comparer)
		{
			return slinq.OrderBy(selector, parameter, Comparisons<K>.ToComparison(comparer), ascending: true);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderBy<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter, Comparison<K> comparison)
		{
			return slinq.OrderBy(selector, parameter, comparison, ascending: true);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderByDescending<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter)
		{
			return slinq.OrderBy(selector, parameter, Comparisons<K>.ToComparison(Smooth.Collections.Comparer<K>.Default), ascending: false);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderByDescending<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter, IComparer<K> comparer)
		{
			return slinq.OrderBy(selector, parameter, Comparisons<K>.ToComparison(comparer), ascending: false);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderByDescending<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter, Comparison<K> comparison)
		{
			return slinq.OrderBy(selector, parameter, comparison, ascending: false);
		}

		public static Slinq<T, LinkedContext<K, T>> OrderBy<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter, Comparison<K> comparison, bool ascending)
		{
			if (!slinq.current.isSome)
			{
				return default(Slinq<T, LinkedContext<K, T>>);
			}
			return Linked.Sort(slinq.ToLinked(selector, parameter), comparison, ascending).SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroup<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector)
		{
			return slinq.ToLookup(selector, Smooth.Collections.EqualityComparer<K>.Default).SortKeys(Comparisons<K>.Default, ascending: true).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroup<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector, IEqualityComparer<K> equalityComparer, IComparer<K> comparer)
		{
			return slinq.ToLookup(selector, equalityComparer).SortKeys(Comparisons<K>.ToComparison(comparer), ascending: true).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroup<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector, IEqualityComparer<K> equalityComparer, Comparison<K> comparison)
		{
			return slinq.ToLookup(selector, equalityComparer).SortKeys(comparison, ascending: true).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroupDescending<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector)
		{
			return slinq.ToLookup(selector, Smooth.Collections.EqualityComparer<K>.Default).SortKeys(Comparisons<K>.Default, ascending: false).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroupDescending<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector, IEqualityComparer<K> equalityComparer, IComparer<K> comparer)
		{
			return slinq.ToLookup(selector, equalityComparer).SortKeys(Comparisons<K>.ToComparison(comparer), ascending: false).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroupDescending<K, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, K> selector, IEqualityComparer<K> equalityComparer, Comparison<K> comparison)
		{
			return slinq.ToLookup(selector, equalityComparer).SortKeys(comparison, ascending: false).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroup<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter)
		{
			return slinq.ToLookup(selector, parameter, Smooth.Collections.EqualityComparer<K>.Default).SortKeys(Comparisons<K>.Default, ascending: true).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroup<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter, IEqualityComparer<K> equalityComparer, IComparer<K> comparer)
		{
			return slinq.ToLookup(selector, parameter, equalityComparer).SortKeys(Comparisons<K>.ToComparison(comparer), ascending: true).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroup<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter, IEqualityComparer<K> equalityComparer, Comparison<K> comparison)
		{
			return slinq.ToLookup(selector, parameter, equalityComparer).SortKeys(comparison, ascending: true).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroupDescending<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter)
		{
			return slinq.ToLookup(selector, parameter, Smooth.Collections.EqualityComparer<K>.Default).SortKeys(Comparisons<K>.Default, ascending: false).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroupDescending<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter, IEqualityComparer<K> equalityComparer, IComparer<K> comparer)
		{
			return slinq.ToLookup(selector, parameter, equalityComparer).SortKeys(Comparisons<K>.ToComparison(comparer), ascending: false).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> OrderByGroupDescending<K, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, K> selector, P parameter, IEqualityComparer<K> equalityComparer, Comparison<K> comparison)
		{
			return slinq.ToLookup(selector, parameter, equalityComparer).SortKeys(comparison, ascending: false).FlattenAndDispose()
				.SlinqAndDispose();
		}

		public static Slinq<T, LinkedContext<T>> Reverse<T, C>(this Slinq<T, C> slinq)
		{
			if (!slinq.current.isSome)
			{
				return default(Slinq<T, LinkedContext<T>>);
			}
			return slinq.ToLinkedReverse().SlinqAndDispose();
		}

		public static Slinq<U, SelectContext<U, T, C>> Select<U, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, U> selector)
		{
			return SelectContext<U, T, C>.Select(slinq, selector);
		}

		public static Slinq<U, SelectContext<U, T, C, P>> Select<U, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, U> selector, P parameter)
		{
			return SelectContext<U, T, C, P>.Select(slinq, selector, parameter);
		}

		public static Slinq<U, SelectSlinqContext<U, UC, T, C>> SelectMany<U, UC, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, Slinq<U, UC>> selector)
		{
			return SelectSlinqContext<U, UC, T, C>.SelectMany(slinq, selector);
		}

		public static Slinq<U, SelectSlinqContext<U, UC, T, C, P>> SelectMany<U, UC, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, Slinq<U, UC>> selector, P parameter)
		{
			return SelectSlinqContext<U, UC, T, C, P>.SelectMany(slinq, selector, parameter);
		}

		public static Slinq<U, SelectOptionContext<U, T, C>> SelectMany<U, T, C>(this Slinq<T, C> slinq, DelegateFunc<T, Option<U>> selector)
		{
			return SelectOptionContext<U, T, C>.SelectMany(slinq, selector);
		}

		public static Slinq<U, SelectOptionContext<U, T, C, P>> SelectMany<U, T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, Option<U>> selector, P parameter)
		{
			return SelectOptionContext<U, T, C, P>.SelectMany(slinq, selector, parameter);
		}

		public static Slinq<T, IntContext<T, C>> Take<T, C>(this Slinq<T, C> slinq, int count)
		{
			return IntContext<T, C>.Take(slinq, count);
		}

		public static Slinq<T, LinkedContext<T>> TakeRight<T, C>(this Slinq<T, C> slinq, int count)
		{
			if (slinq.current.isSome)
			{
				Smooth.Algebraics.Tuple<LinkedHeadTail<T>, LinkedHeadTail<T>> tuple = slinq.SplitRight(count);
				tuple.Item1.DisposeInBackground();
				return tuple.Item2.SlinqAndDispose();
			}
			return default(Slinq<T, LinkedContext<T>>);
		}

		public static Slinq<T, PredicateContext<T, C>> TakeWhile<T, C>(this Slinq<T, C> slinq, DelegateFunc<T, bool> predicate)
		{
			return PredicateContext<T, C>.TakeWhile(slinq, predicate);
		}

		public static Slinq<T, PredicateContext<T, C, P>> TakeWhile<T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, bool> predicate, P parameter)
		{
			return PredicateContext<T, C, P>.TakeWhile(slinq, predicate, parameter);
		}

		public static Slinq<T, PredicateContext<T, C>> Where<T, C>(this Slinq<T, C> slinq, DelegateFunc<T, bool> predicate)
		{
			return PredicateContext<T, C>.Where(slinq, predicate);
		}

		public static Slinq<T, PredicateContext<T, C, P>> Where<T, C, P>(this Slinq<T, C> slinq, DelegateFunc<T, P, bool> predicate, P parameter)
		{
			return PredicateContext<T, C, P>.Where(slinq, predicate, parameter);
		}

		public static Slinq<T, HashSetContext<T, ConcatContext<C2, T, C>>> Union<C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other)
		{
			return slinq.Concat(other).Distinct(Smooth.Collections.EqualityComparer<T>.Default);
		}

		public static Slinq<T, HashSetContext<T, ConcatContext<C2, T, C>>> Union<C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other, IEqualityComparer<T> comparer)
		{
			return slinq.Concat(other).Distinct(comparer);
		}

		public static Slinq<T, HashSetContext<K, T, ConcatContext<C2, T, C>>> Union<K, C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, K> selector)
		{
			return slinq.Concat(other).Distinct(selector, Smooth.Collections.EqualityComparer<K>.Default);
		}

		public static Slinq<T, HashSetContext<K, T, ConcatContext<C2, T, C>>> Union<K, C2, T, C>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, K> selector, IEqualityComparer<K> comparer)
		{
			return slinq.Concat(other).Distinct(selector, comparer);
		}

		public static Slinq<T, HashSetContext<K, T, ConcatContext<C2, T, C>, P>> Union<K, C2, T, C, P>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, P, K> selector, P parameter)
		{
			return slinq.Concat(other).Distinct(selector, parameter, Smooth.Collections.EqualityComparer<K>.Default);
		}

		public static Slinq<T, HashSetContext<K, T, ConcatContext<C2, T, C>, P>> Union<K, C2, T, C, P>(this Slinq<T, C> slinq, Slinq<T, C2> other, DelegateFunc<T, P, K> selector, P parameter, IEqualityComparer<K> comparer)
		{
			return slinq.Concat(other).Distinct(selector, parameter, comparer);
		}

		public static Slinq<Smooth.Algebraics.Tuple<T, T2>, ZipContext<T2, C2, T, C>> Zip<T2, C2, T, C>(this Slinq<T, C> slinq, Slinq<T2, C2> with)
		{
			return ZipContext<T2, C2, T, C>.Zip(slinq, with, ZipRemoveFlags.Left);
		}

		public static Slinq<Smooth.Algebraics.Tuple<T, T2>, ZipContext<T2, C2, T, C>> Zip<T2, C2, T, C>(this Slinq<T, C> slinq, Slinq<T2, C2> with, ZipRemoveFlags removeFlags)
		{
			return ZipContext<T2, C2, T, C>.Zip(slinq, with, removeFlags);
		}

		public static Slinq<U, ZipContext<U, T2, C2, T, C>> Zip<U, T2, C2, T, C>(this Slinq<T, C> slinq, Slinq<T2, C2> with, DelegateFunc<T, T2, U> selector)
		{
			return ZipContext<U, T2, C2, T, C>.Zip(slinq, with, selector, ZipRemoveFlags.Left);
		}

		public static Slinq<U, ZipContext<U, T2, C2, T, C>> Zip<U, T2, C2, T, C>(this Slinq<T, C> slinq, Slinq<T2, C2> with, DelegateFunc<T, T2, U> selector, ZipRemoveFlags removeFlags)
		{
			return ZipContext<U, T2, C2, T, C>.Zip(slinq, with, selector, removeFlags);
		}

		public static Slinq<U, ZipContext<U, T2, C2, T, C, P>> Zip<U, T2, C2, T, C, P>(this Slinq<T, C> slinq, Slinq<T2, C2> with, DelegateFunc<T, T2, P, U> selector, P parameter)
		{
			return ZipContext<U, T2, C2, T, C, P>.Zip(slinq, with, selector, parameter, ZipRemoveFlags.Left);
		}

		public static Slinq<U, ZipContext<U, T2, C2, T, C, P>> Zip<U, T2, C2, T, C, P>(this Slinq<T, C> slinq, Slinq<T2, C2> with, DelegateFunc<T, T2, P, U> selector, P parameter, ZipRemoveFlags removeFlags)
		{
			return ZipContext<U, T2, C2, T, C, P>.Zip(slinq, with, selector, parameter, removeFlags);
		}

		public static Slinq<Smooth.Algebraics.Tuple<Option<T>, Option<T2>>, ZipAllContext<T2, C2, T, C>> ZipAll<T2, C2, T, C>(this Slinq<T, C> slinq, Slinq<T2, C2> with)
		{
			return ZipAllContext<T2, C2, T, C>.ZipAll(slinq, with, ZipRemoveFlags.Left);
		}

		public static Slinq<Smooth.Algebraics.Tuple<Option<T>, Option<T2>>, ZipAllContext<T2, C2, T, C>> ZipAll<T2, C2, T, C>(this Slinq<T, C> slinq, Slinq<T2, C2> with, ZipRemoveFlags removeFlags)
		{
			return ZipAllContext<T2, C2, T, C>.ZipAll(slinq, with, removeFlags);
		}

		public static Slinq<U, ZipAllContext<U, T2, C2, T, C>> ZipAll<U, T2, C2, T, C>(this Slinq<T, C> slinq, Slinq<T2, C2> with, DelegateFunc<Option<T>, Option<T2>, U> selector)
		{
			return ZipAllContext<U, T2, C2, T, C>.ZipAll(slinq, with, selector, ZipRemoveFlags.Left);
		}

		public static Slinq<U, ZipAllContext<U, T2, C2, T, C>> ZipAll<U, T2, C2, T, C>(this Slinq<T, C> slinq, Slinq<T2, C2> with, DelegateFunc<Option<T>, Option<T2>, U> selector, ZipRemoveFlags removeFlags)
		{
			return ZipAllContext<U, T2, C2, T, C>.ZipAll(slinq, with, selector, removeFlags);
		}

		public static Slinq<U, ZipAllContext<U, T2, C2, T, C, P>> ZipAll<U, T2, C2, T, C, P>(this Slinq<T, C> slinq, Slinq<T2, C2> with, DelegateFunc<Option<T>, Option<T2>, P, U> selector, P parameter)
		{
			return ZipAllContext<U, T2, C2, T, C, P>.ZipAll(slinq, with, selector, parameter, ZipRemoveFlags.Left);
		}

		public static Slinq<U, ZipAllContext<U, T2, C2, T, C, P>> ZipAll<U, T2, C2, T, C, P>(this Slinq<T, C> slinq, Slinq<T2, C2> with, DelegateFunc<Option<T>, Option<T2>, P, U> selector, P parameter, ZipRemoveFlags removeFlags)
		{
			return ZipAllContext<U, T2, C2, T, C, P>.ZipAll(slinq, with, selector, parameter, removeFlags);
		}

		public static Slinq<Smooth.Algebraics.Tuple<T, int>, ZipContext<int, FuncContext<int, int>, T, C>> ZipWithIndex<T, C>(this Slinq<T, C> slinq)
		{
			return ZipContext<int, FuncContext<int, int>, T, C>.Zip(slinq, Slinqable.Sequence(0, 1), ZipRemoveFlags.Left);
		}

		public static double Average<C>(this Slinq<int, C> slinq)
		{
			return slinq.AverageOrNone().ValueOr(delegate
			{
				throw new InvalidOperationException();
			});
		}

		public static double Average<C>(this Slinq<long, C> slinq)
		{
			return slinq.AverageOrNone().ValueOr(delegate
			{
				throw new InvalidOperationException();
			});
		}

		public static float Average<C>(this Slinq<float, C> slinq)
		{
			return slinq.AverageOrNone().ValueOr(delegate
			{
				throw new InvalidOperationException();
			});
		}

		public static double Average<C>(this Slinq<double, C> slinq)
		{
			return slinq.AverageOrNone().ValueOr(delegate
			{
				throw new InvalidOperationException();
			});
		}

		public static Option<double> AverageOrNone<C>(this Slinq<int, C> slinq)
		{
			if (slinq.current.isSome)
			{
				int num = 0;
				double num2 = 0.0;
				while (slinq.current.isSome)
				{
					num2 += (double)slinq.current.value;
					slinq.skip(ref slinq.context, out slinq.current);
					num++;
				}
				return new Option<double>(num2 / (double)num);
			}
			return default(Option<double>);
		}

		public static Option<double> AverageOrNone<C>(this Slinq<long, C> slinq)
		{
			if (slinq.current.isSome)
			{
				int num = 0;
				double num2 = 0.0;
				while (slinq.current.isSome)
				{
					num2 += (double)slinq.current.value;
					slinq.skip(ref slinq.context, out slinq.current);
					num++;
				}
				return new Option<double>(num2 / (double)num);
			}
			return default(Option<double>);
		}

		public static Option<float> AverageOrNone<C>(this Slinq<float, C> slinq)
		{
			if (slinq.current.isSome)
			{
				int num = 0;
				float num2 = 0f;
				while (slinq.current.isSome)
				{
					num2 += slinq.current.value;
					slinq.skip(ref slinq.context, out slinq.current);
					num++;
				}
				return new Option<float>(num2 / (float)num);
			}
			return default(Option<float>);
		}

		public static Option<double> AverageOrNone<C>(this Slinq<double, C> slinq)
		{
			if (slinq.current.isSome)
			{
				int num = 0;
				double num2 = 0.0;
				while (slinq.current.isSome)
				{
					num2 += slinq.current.value;
					slinq.skip(ref slinq.context, out slinq.current);
					num++;
				}
				return new Option<double>(num2 / (double)num);
			}
			return default(Option<double>);
		}

		public static int Sum<C>(this Slinq<int, C> slinq)
		{
			int num = 0;
			while (slinq.current.isSome)
			{
				num = checked(num + slinq.current.value);
				slinq.skip(ref slinq.context, out slinq.current);
			}
			return num;
		}

		public static long Sum<C>(this Slinq<long, C> slinq)
		{
			long num = 0L;
			while (slinq.current.isSome)
			{
				num = checked(num + slinq.current.value);
				slinq.skip(ref slinq.context, out slinq.current);
			}
			return num;
		}

		public static float Sum<C>(this Slinq<float, C> slinq)
		{
			float num = 0f;
			while (slinq.current.isSome)
			{
				num += slinq.current.value;
				slinq.skip(ref slinq.context, out slinq.current);
			}
			return num;
		}

		public static double Sum<C>(this Slinq<double, C> slinq)
		{
			double num = 0.0;
			while (slinq.current.isSome)
			{
				num += slinq.current.value;
				slinq.skip(ref slinq.context, out slinq.current);
			}
			return num;
		}
	}
}
