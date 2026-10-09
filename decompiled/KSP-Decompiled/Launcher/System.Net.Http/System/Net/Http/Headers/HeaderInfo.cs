using System.Collections.Generic;

namespace System.Net.Http.Headers;

internal abstract class HeaderInfo
{
	private class HeaderTypeInfo<T, U> : HeaderInfo where U : class
	{
		private readonly TryParseDelegate<T> parser;

		public HeaderTypeInfo(string name, TryParseDelegate<T> parser, HttpHeaderKind headerKind)
			: base(name, headerKind)
		{
			this.parser = parser;
		}

		public override void AddToCollection(object collection, object value)
		{
			HttpHeaderValueCollection<U> httpHeaderValueCollection = (HttpHeaderValueCollection<U>)collection;
			if (value is List<U> values)
			{
				httpHeaderValueCollection.AddRange(values);
			}
			else
			{
				httpHeaderValueCollection.Add((U)value);
			}
		}

		protected override object CreateCollection(HttpHeaders headers, HeaderInfo headerInfo)
		{
			return new HttpHeaderValueCollection<U>(headers, headerInfo);
		}

		public override List<string> ToStringCollection(object collection)
		{
			if (collection == null)
			{
				return null;
			}
			HttpHeaderValueCollection<U> httpHeaderValueCollection = (HttpHeaderValueCollection<U>)collection;
			if (httpHeaderValueCollection.Count == 0)
			{
				if (httpHeaderValueCollection.InvalidValues == null)
				{
					return null;
				}
				return new List<string>(httpHeaderValueCollection.InvalidValues);
			}
			List<string> list = new List<string>();
			foreach (U item in httpHeaderValueCollection)
			{
				list.Add(item.ToString());
			}
			if (httpHeaderValueCollection.InvalidValues != null)
			{
				list.AddRange(httpHeaderValueCollection.InvalidValues);
			}
			return list;
		}

		public override bool TryParse(string value, out object result)
		{
			T result2;
			bool result3 = parser(value, out result2);
			result = result2;
			return result3;
		}
	}

	private class CollectionHeaderTypeInfo<T, U> : HeaderTypeInfo<T, U> where U : class
	{
		private readonly int minimalCount;

		private readonly string separator;

		private readonly TryParseListDelegate<T> parser;

		public override string Separator => separator;

		public CollectionHeaderTypeInfo(string name, TryParseListDelegate<T> parser, HttpHeaderKind headerKind, int minimalCount, string separator)
			: base(name, (TryParseDelegate<T>)null, headerKind)
		{
			this.parser = parser;
			this.minimalCount = minimalCount;
			AllowsMany = true;
			this.separator = separator;
		}

		public override bool TryParse(string value, out object result)
		{
			if (!parser(value, minimalCount, out var result2))
			{
				result = null;
				return false;
			}
			result = result2;
			return true;
		}
	}

	public bool AllowsMany;

	public readonly HttpHeaderKind HeaderKind;

	public readonly string Name;

	public Func<object, string> CustomToString { get; private set; }

	public virtual string Separator
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	protected HeaderInfo(string name, HttpHeaderKind headerKind)
	{
		Name = name;
		HeaderKind = headerKind;
	}

	public static HeaderInfo CreateSingle<T>(string name, TryParseDelegate<T> parser, HttpHeaderKind headerKind, Func<object, string> toString = null)
	{
		return new HeaderTypeInfo<T, object>(name, parser, headerKind)
		{
			CustomToString = toString
		};
	}

	public static HeaderInfo CreateMulti<T>(string name, TryParseListDelegate<T> elementParser, HttpHeaderKind headerKind, int minimalCount = 1, string separator = ", ") where T : class
	{
		return new CollectionHeaderTypeInfo<T, T>(name, elementParser, headerKind, minimalCount, separator);
	}

	public object CreateCollection(HttpHeaders headers)
	{
		return CreateCollection(headers, this);
	}

	public abstract void AddToCollection(object collection, object value);

	protected abstract object CreateCollection(HttpHeaders headers, HeaderInfo headerInfo);

	public abstract List<string> ToStringCollection(object collection);

	public abstract bool TryParse(string value, out object result);
}
