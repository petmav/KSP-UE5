using System.Collections.Generic;

namespace Smooth.Pools
{
	public static class ListPool<T>
	{
		private static readonly Pool<List<T>> _Instance = new Pool<List<T>>(() => new List<T>(), delegate(List<T> list)
		{
			list.Clear();
		});

		public static Pool<List<T>> Instance => _Instance;
	}
}
