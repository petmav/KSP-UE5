using System.Collections;
using Unity;

namespace System.Diagnostics;

public class EventLogEntryCollection : ICollection, IEnumerable
{
	private class EventLogEntryEnumerator : IEnumerator
	{
		private readonly EventLogImpl _impl;

		private int _currentIndex = -1;

		private EventLogEntry _currentEntry;

		object IEnumerator.Current => Current;

		public EventLogEntry Current
		{
			get
			{
				if (_currentEntry != null)
				{
					return _currentEntry;
				}
				throw new InvalidOperationException("No current EventLog entry available, cursor is located before the first or after the last element of the enumeration.");
			}
		}

		internal EventLogEntryEnumerator(EventLogImpl impl)
		{
			_impl = impl;
		}

		public bool MoveNext()
		{
			_currentIndex++;
			if (_currentIndex >= _impl.EntryCount)
			{
				_currentEntry = null;
				return false;
			}
			_currentEntry = _impl[_currentIndex];
			return true;
		}

		public void Reset()
		{
			_currentIndex = -1;
		}
	}

	private readonly EventLogImpl _impl;

	public int Count => _impl.EntryCount;

	public virtual EventLogEntry this[int index] => _impl[index];

	bool ICollection.IsSynchronized => false;

	object ICollection.SyncRoot => this;

	internal EventLogEntryCollection(EventLogImpl impl)
	{
		_impl = impl;
	}

	public void CopyTo(EventLogEntry[] entries, int index)
	{
		EventLogEntry[] entries2 = _impl.GetEntries();
		Array.Copy(entries2, 0, entries, index, entries2.Length);
	}

	public IEnumerator GetEnumerator()
	{
		return new EventLogEntryEnumerator(_impl);
	}

	void ICollection.CopyTo(Array array, int index)
	{
		EventLogEntry[] entries = _impl.GetEntries();
		Array.Copy(entries, 0, array, index, entries.Length);
	}

	internal EventLogEntryCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
