using System;
using Smooth.Dispose;

namespace Smooth.Slinq.Collections
{
	public class Linked<T> : IDisposable
	{
		private static object poolLock = new object();

		private static Linked<T> pool;

		public Linked<T> next;

		public T value;

		private Linked()
		{
		}

		public static Linked<T> Borrow(T value)
		{
			Linked<T> linked;
			lock (poolLock)
			{
				if (pool == null)
				{
					linked = new Linked<T>();
				}
				else
				{
					linked = pool;
					pool = pool.next;
					linked.next = null;
				}
			}
			linked.value = value;
			return linked;
		}

		public void TrimAndDispose()
		{
			value = default(T);
			lock (poolLock)
			{
				next = pool;
				pool = this;
			}
		}

		public void DisposeInBackground()
		{
			DisposalQueue.Enqueue(this);
		}

		public void Dispose()
		{
			T val = (value = default(T));
			Linked<T> linked = this;
			while (linked.next != null)
			{
				linked = linked.next;
				linked.value = val;
			}
			lock (poolLock)
			{
				linked.next = pool;
				pool = this;
			}
		}
	}
	public class Linked<K, T> : IDisposable
	{
		private static object poolLock = new object();

		private static Linked<K, T> pool;

		public Linked<K, T> next;

		public K key;

		public T value;

		private Linked()
		{
		}

		public static Linked<K, T> Borrow(K key, T value)
		{
			Linked<K, T> linked;
			lock (poolLock)
			{
				if (pool == null)
				{
					linked = new Linked<K, T>();
				}
				else
				{
					linked = pool;
					pool = pool.next;
					linked.next = null;
				}
			}
			linked.key = key;
			linked.value = value;
			return linked;
		}

		public void TrimAndDispose()
		{
			key = default(K);
			value = default(T);
			lock (poolLock)
			{
				next = pool;
				pool = this;
			}
		}

		public void DisposeInBackground()
		{
			DisposalQueue.Enqueue(this);
		}

		public void Dispose()
		{
			K val = default(K);
			T val2 = default(T);
			key = val;
			value = val2;
			Linked<K, T> linked = this;
			while (linked.next != null)
			{
				linked = linked.next;
				linked.key = val;
				linked.value = val2;
			}
			lock (poolLock)
			{
				linked.next = pool;
				pool = this;
			}
		}
	}
	public static class Linked
	{
		public static LinkedHeadTail<T> Reverse<T>(this LinkedHeadTail<T> list)
		{
			LinkedHeadTail<T> result = default(LinkedHeadTail<T>);
			result.tail = list.head;
			result.count = list.count;
			while (list.head != null)
			{
				Linked<T> head = list.head;
				list.head = list.head.next;
				head.next = result.head;
				result.head = head;
			}
			return result;
		}

		public static LinkedHeadTail<K, T> Reverse<K, T>(this LinkedHeadTail<K, T> list)
		{
			LinkedHeadTail<K, T> result = default(LinkedHeadTail<K, T>);
			result.tail = list.head;
			result.count = list.count;
			while (list.head != null)
			{
				Linked<K, T> head = list.head;
				list.head = list.head.next;
				head.next = result.head;
				result.head = head;
			}
			return result;
		}

		public static LinkedHeadTail<T> Sort<T>(LinkedHeadTail<T> input, Comparison<T> comparison, bool ascending)
		{
			if (input.count <= 1)
			{
				return input;
			}
			if (input.count == 2)
			{
				if ((ascending ? comparison(input.head.value, input.tail.value) : comparison(input.tail.value, input.head.value)) <= 0)
				{
					return input;
				}
				input.head.next = null;
				input.tail.next = input.head;
				input.head = input.tail;
				input.tail = input.head.next;
				return input;
			}
			if (input.count == 3)
			{
				int num = (ascending ? 1 : (-1));
				Linked<T> next = input.head.next;
				if (num * comparison(input.head.value, next.value) <= 0)
				{
					if (num * comparison(next.value, input.tail.value) > 0)
					{
						if (num * comparison(input.head.value, input.tail.value) <= 0)
						{
							input.tail.next = next;
							input.head.next = input.tail;
							input.tail = next;
							next.next = null;
						}
						else
						{
							input.tail.next = input.head;
							input.head = input.tail;
							input.tail = next;
							next.next = null;
						}
					}
				}
				else if (num * comparison(next.value, input.tail.value) <= 0)
				{
					if (num * comparison(input.head.value, input.tail.value) <= 0)
					{
						input.head.next = input.tail;
						next.next = input.head;
						input.head = next;
					}
					else
					{
						input.tail.next = input.head;
						input.tail = input.head;
						input.tail.next = null;
						input.head = next;
					}
				}
				else
				{
					input.tail = input.head;
					input.head = next.next;
					input.head.next = next;
					next.next = input.tail;
					input.tail.next = null;
				}
				return input;
			}
			LinkedHeadTail<T> input2 = default(LinkedHeadTail<T>);
			LinkedHeadTail<T> input3 = default(LinkedHeadTail<T>);
			input2.count = input.count / 2;
			input3.count = input.count - input2.count;
			input2.head = input.head;
			input2.tail = input.head;
			input3.tail = input.tail;
			for (int i = 1; i < input2.count; i++)
			{
				input2.tail = input2.tail.next;
			}
			input3.head = input2.tail.next;
			input2.tail.next = null;
			return Merge(Sort(input2, comparison, ascending), Sort(input3, comparison, ascending), comparison, ascending);
		}

		public static LinkedHeadTail<K, T> Sort<K, T>(LinkedHeadTail<K, T> input, Comparison<K> comparison, bool ascending)
		{
			if (input.count <= 1)
			{
				return input;
			}
			if (input.count == 2)
			{
				if ((ascending ? comparison(input.head.key, input.tail.key) : comparison(input.tail.key, input.head.key)) <= 0)
				{
					return input;
				}
				input.head.next = null;
				input.tail.next = input.head;
				input.head = input.tail;
				input.tail = input.head.next;
				return input;
			}
			if (input.count == 3)
			{
				int num = (ascending ? 1 : (-1));
				Linked<K, T> next = input.head.next;
				if (num * comparison(input.head.key, next.key) <= 0)
				{
					if (num * comparison(next.key, input.tail.key) > 0)
					{
						if (num * comparison(input.head.key, input.tail.key) <= 0)
						{
							input.tail.next = next;
							input.head.next = input.tail;
							input.tail = next;
							next.next = null;
						}
						else
						{
							input.tail.next = input.head;
							input.head = input.tail;
							input.tail = next;
							next.next = null;
						}
					}
				}
				else if (num * comparison(next.key, input.tail.key) <= 0)
				{
					if (num * comparison(input.head.key, input.tail.key) <= 0)
					{
						input.head.next = input.tail;
						next.next = input.head;
						input.head = next;
					}
					else
					{
						input.tail.next = input.head;
						input.tail = input.head;
						input.tail.next = null;
						input.head = next;
					}
				}
				else
				{
					input.tail = input.head;
					input.head = next.next;
					input.head.next = next;
					next.next = input.tail;
					input.tail.next = null;
				}
				return input;
			}
			LinkedHeadTail<K, T> input2 = default(LinkedHeadTail<K, T>);
			LinkedHeadTail<K, T> input3 = default(LinkedHeadTail<K, T>);
			input2.count = input.count / 2;
			input3.count = input.count - input2.count;
			input2.head = input.head;
			input2.tail = input.head;
			input3.tail = input.tail;
			for (int i = 1; i < input2.count; i++)
			{
				input2.tail = input2.tail.next;
			}
			input3.head = input2.tail.next;
			input2.tail.next = null;
			return Merge(Sort(input2, comparison, ascending), Sort(input3, comparison, ascending), comparison, ascending);
		}

		public static LinkedHeadTail<T> Merge<T>(LinkedHeadTail<T> left, LinkedHeadTail<T> right, Comparison<T> comparison, bool ascending)
		{
			if (left.count == 0)
			{
				return right;
			}
			if (right.count == 0)
			{
				return left;
			}
			int num = (ascending ? 1 : (-1));
			if (num * comparison(left.tail.value, right.head.value) <= 0)
			{
				left.tail.next = right.head;
				left.tail = right.tail;
				left.count += right.count;
				return left;
			}
			if (num * comparison(left.head.value, right.tail.value) > 0)
			{
				right.tail.next = left.head;
				right.tail = left.tail;
				right.count += left.count;
				return right;
			}
			LinkedHeadTail<T> result = default(LinkedHeadTail<T>);
			result.count = left.count + right.count;
			if (num * comparison(left.head.value, right.head.value) <= 0)
			{
				result.head = left.head;
				result.tail = left.head;
				left.head = left.head.next;
			}
			else
			{
				result.head = right.head;
				result.tail = right.head;
				right.head = right.head.next;
			}
			while (left.head != null && right.head != null)
			{
				if (num * comparison(left.head.value, right.head.value) <= 0)
				{
					result.tail.next = left.head;
					result.tail = left.head;
					left.head = left.head.next;
				}
				else
				{
					result.tail.next = right.head;
					result.tail = right.head;
					right.head = right.head.next;
				}
			}
			if (left.head == null)
			{
				result.tail.next = right.head;
				result.tail = right.tail;
			}
			else
			{
				result.tail.next = left.head;
				result.tail = left.tail;
			}
			return result;
		}

		public static LinkedHeadTail<K, T> Merge<K, T>(LinkedHeadTail<K, T> left, LinkedHeadTail<K, T> right, Comparison<K> comparison, bool ascending)
		{
			if (left.count == 0)
			{
				return right;
			}
			if (right.count == 0)
			{
				return left;
			}
			int num = (ascending ? 1 : (-1));
			if (num * comparison(left.tail.key, right.head.key) <= 0)
			{
				left.tail.next = right.head;
				left.tail = right.tail;
				left.count += right.count;
				return left;
			}
			if (num * comparison(left.head.key, right.tail.key) > 0)
			{
				right.tail.next = left.head;
				right.tail = left.tail;
				right.count += left.count;
				return right;
			}
			LinkedHeadTail<K, T> result = default(LinkedHeadTail<K, T>);
			result.count = left.count + right.count;
			if (num * comparison(left.head.key, right.head.key) <= 0)
			{
				result.head = left.head;
				result.tail = left.head;
				left.head = left.head.next;
			}
			else
			{
				result.head = right.head;
				result.tail = right.head;
				right.head = right.head.next;
			}
			while (left.head != null && right.head != null)
			{
				if (num * comparison(left.head.key, right.head.key) <= 0)
				{
					result.tail.next = left.head;
					result.tail = left.head;
					left.head = left.head.next;
				}
				else
				{
					result.tail.next = right.head;
					result.tail = right.head;
					right.head = right.head.next;
				}
			}
			if (left.head == null)
			{
				result.tail.next = right.head;
				result.tail = right.tail;
			}
			else
			{
				result.tail.next = left.head;
				result.tail = left.tail;
			}
			return result;
		}

		public static LinkedHeadTail<T> InsertionSort<T>(LinkedHeadTail<T> input, Comparison<T> comparison, bool ascending)
		{
			if (input.count <= 1)
			{
				return input;
			}
			int num = (ascending ? 1 : (-1));
			LinkedHeadTail<T> result = default(LinkedHeadTail<T>);
			result.count = input.count;
			Linked<T> head = input.head;
			input.head = input.head.next;
			head.next = null;
			result.head = head;
			result.tail = head;
			T y = head.value;
			T y2 = head.value;
			while (input.head != null)
			{
				head = input.head;
				input.head = input.head.next;
				T value = head.value;
				if (num * comparison(value, y2) >= 0)
				{
					y2 = value;
					head.next = null;
					result.tail.next = head;
					result.tail = head;
					continue;
				}
				if (num * comparison(value, y) < 0)
				{
					y = value;
					head.next = result.head;
					result.head = head;
					continue;
				}
				Linked<T> linked = result.head;
				while (linked.next != null && num * comparison(value, linked.next.value) >= 0)
				{
					linked = linked.next;
				}
				head.next = linked.next;
				linked.next = head;
			}
			return result;
		}

		public static LinkedHeadTail<K, T> InsertionSort<K, T>(LinkedHeadTail<K, T> input, Comparison<K> comparison, bool ascending)
		{
			if (input.count <= 1)
			{
				return input;
			}
			int num = (ascending ? 1 : (-1));
			LinkedHeadTail<K, T> result = default(LinkedHeadTail<K, T>);
			result.count = input.count;
			Linked<K, T> head = input.head;
			input.head = input.head.next;
			head.next = null;
			result.head = head;
			result.tail = head;
			K y = head.key;
			K y2 = head.key;
			while (input.head != null)
			{
				head = input.head;
				input.head = input.head.next;
				K key = head.key;
				if (num * comparison(key, y2) >= 0)
				{
					y2 = key;
					head.next = null;
					result.tail.next = head;
					result.tail = head;
					continue;
				}
				if (num * comparison(key, y) < 0)
				{
					y = key;
					head.next = result.head;
					result.head = head;
					continue;
				}
				Linked<K, T> linked = result.head;
				while (linked.next != null && num * comparison(key, linked.next.key) >= 0)
				{
					linked = linked.next;
				}
				head.next = linked.next;
				linked.next = head;
			}
			return result;
		}
	}
}
