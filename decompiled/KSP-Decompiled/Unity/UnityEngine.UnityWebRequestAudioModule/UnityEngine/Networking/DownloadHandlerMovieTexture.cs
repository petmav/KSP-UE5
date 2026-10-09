using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using UnityEngine.Bindings;

namespace UnityEngine.Networking;

[StructLayout(LayoutKind.Sequential)]
[NativeHeader("Runtime/Video/MovieTexture.h")]
[NativeHeader("Modules/UnityWebRequestAudio/Public/DownloadHandlerMovieTexture.h")]
[Obsolete("MovieTexture is deprecated. Use VideoPlayer instead.", false)]
public sealed class DownloadHandlerMovieTexture : DownloadHandler
{
	public extern MovieTexture movieTexture
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
	}

	public DownloadHandlerMovieTexture()
	{
		InternalCreateDHMovieTexture();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern IntPtr Create(DownloadHandlerMovieTexture obj);

	private void InternalCreateDHMovieTexture()
	{
		m_Ptr = Create(this);
	}

	protected override byte[] GetData()
	{
		return DownloadHandler.InternalGetByteArray(this);
	}

	protected override string GetText()
	{
		throw new NotSupportedException("String access is not supported for movies");
	}

	public static MovieTexture GetContent(UnityWebRequest uwr)
	{
		return DownloadHandler.GetCheckedDownloader<DownloadHandlerMovieTexture>(uwr).movieTexture;
	}
}
