using System.Collections;
using System.ComponentModel;
using System.Drawing.Imaging;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Drawing.Printing;

[Serializable]
public class PrinterSettings : ICloneable
{
	public class PaperSizeCollection : ICollection, IEnumerable
	{
		public int Count
		{
			get
			{
				Unity.ThrowStub.ThrowNotSupportedException();
				return default(int);
			}
		}

		public virtual PaperSize this[int index]
		{
			get
			{
				Unity.ThrowStub.ThrowNotSupportedException();
				return null;
			}
		}

		public PaperSizeCollection(PaperSize[] array)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		[SpecialName]
		int ICollection.get_Count()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}

		[SpecialName]
		bool ICollection.get_IsSynchronized()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}

		[SpecialName]
		object ICollection.get_SyncRoot()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		public int Add(PaperSize paperSize)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}

		public void CopyTo(PaperSize[] paperSizes, int index)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		public IEnumerator GetEnumerator()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}

		void ICollection.CopyTo(Array array, int index)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public class PaperSourceCollection : ICollection, IEnumerable
	{
		public int Count
		{
			get
			{
				Unity.ThrowStub.ThrowNotSupportedException();
				return default(int);
			}
		}

		public virtual PaperSource this[int index]
		{
			get
			{
				Unity.ThrowStub.ThrowNotSupportedException();
				return null;
			}
		}

		public PaperSourceCollection(PaperSource[] array)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		[SpecialName]
		int ICollection.get_Count()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}

		[SpecialName]
		bool ICollection.get_IsSynchronized()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}

		[SpecialName]
		object ICollection.get_SyncRoot()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		public int Add(PaperSource paperSource)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}

		public void CopyTo(PaperSource[] paperSources, int index)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		public IEnumerator GetEnumerator()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}

		void ICollection.CopyTo(Array array, int index)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public class PrinterResolutionCollection : ICollection, IEnumerable
	{
		public int Count
		{
			get
			{
				Unity.ThrowStub.ThrowNotSupportedException();
				return default(int);
			}
		}

		public virtual PrinterResolution this[int index]
		{
			get
			{
				Unity.ThrowStub.ThrowNotSupportedException();
				return null;
			}
		}

		public PrinterResolutionCollection(PrinterResolution[] array)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		[SpecialName]
		int ICollection.get_Count()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}

		[SpecialName]
		bool ICollection.get_IsSynchronized()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}

		[SpecialName]
		object ICollection.get_SyncRoot()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		public int Add(PrinterResolution printerResolution)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}

		public void CopyTo(PrinterResolution[] printerResolutions, int index)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		public IEnumerator GetEnumerator()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}

		void ICollection.CopyTo(Array array, int index)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public class StringCollection : ICollection, IEnumerable
	{
		public int Count
		{
			get
			{
				Unity.ThrowStub.ThrowNotSupportedException();
				return default(int);
			}
		}

		public virtual string this[int index]
		{
			get
			{
				Unity.ThrowStub.ThrowNotSupportedException();
				return null;
			}
		}

		public StringCollection(string[] array)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		[SpecialName]
		int ICollection.get_Count()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}

		[SpecialName]
		bool ICollection.get_IsSynchronized()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}

		[SpecialName]
		object ICollection.get_SyncRoot()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		public int Add(string value)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}

		public void CopyTo(string[] strings, int index)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		public IEnumerator GetEnumerator()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}

		void ICollection.CopyTo(Array array, int index)
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool CanDuplex
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool Collate
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public short Copies
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(short);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PageSettings DefaultPageSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Duplex Duplex
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Duplex);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int FromPage
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public static StringCollection InstalledPrinters
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool IsDefaultPrinter
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsPlotter
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsValid
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public int LandscapeAngle
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public int MaximumCopies
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public int MaximumPage
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int MinimumPage
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PaperSizeCollection PaperSizes
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public PaperSourceCollection PaperSources
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string PrinterName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PrinterResolutionCollection PrinterResolutions
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string PrintFileName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PrintRange PrintRange
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PrintRange);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool PrintToFile
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool SupportsColor
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public int ToPage
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PrinterSettings()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public Graphics CreateMeasurementGraphics()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public Graphics CreateMeasurementGraphics(bool honorOriginAtMargins)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public Graphics CreateMeasurementGraphics(PageSettings pageSettings)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public Graphics CreateMeasurementGraphics(PageSettings pageSettings, bool honorOriginAtMargins)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public IntPtr GetHdevmode()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	public IntPtr GetHdevmode(PageSettings pageSettings)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	public IntPtr GetHdevnames()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	public bool IsDirectPrintingSupported(Image image)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsDirectPrintingSupported(ImageFormat imageFormat)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void SetHdevmode(IntPtr hdevmode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetHdevnames(IntPtr hdevnames)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
