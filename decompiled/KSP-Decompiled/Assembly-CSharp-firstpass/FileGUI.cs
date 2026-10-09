using System.Collections.Generic;
using System.IO;
using UnityEngine;

public class FileGUI : MonoBehaviour
{
	public int DialogueWidth = 600;

	public int DialogueHeight = 384;

	private GUISkin FileIO_Skin;

	private Texture2D Icn_ProjectLoad;

	private Texture2D Icn_Folder;

	private Texture2D Icn_SmallFolder;

	private Texture2D Icn_ParentFolder;

	private Texture2D Icn_File;

	private Texture2D Icn_File_Project;

	private Texture2D Icn_File_3d;

	private Texture2D Icn_File_Image;

	private Texture2D Icn_File_Movie;

	private Texture2D Icn_File_Audio;

	private Texture2D Icon;

	private Vector2 scrollPosition = Vector2.zero;

	private Rect DialogueBox;

	private List<string> extnList = new List<string>();

	private string showext;

	private string[] drives;

	private bool drivelevel;

	private DirectoryInfo oldDI;

	private List<string> validdrives = new List<string>();

	private string newprojectname = "";

	private string err = "";

	private string sep;

	private void Start()
	{
		newprojectname = "";
		err = "";
		sep = Path.DirectorySeparatorChar.ToString();
		FileIO_Skin = (GUISkin)Resources.Load("FileIO_Skin");
		Icn_ProjectLoad = (Texture2D)Resources.Load("FILEIO_ProjectLoadLogo_img");
		Icn_Folder = (Texture2D)Resources.Load("FILEIO_folder_img");
		Icn_SmallFolder = (Texture2D)Resources.Load("FILEIO_folder_s_img");
		Icn_ParentFolder = (Texture2D)Resources.Load("FILEIO_folder_ps_img");
		Icn_File = (Texture2D)Resources.Load("FILEIO_file_img");
		Icn_File_Project = (Texture2D)Resources.Load("FILEIO_file_project_img");
		Icn_File_3d = (Texture2D)Resources.Load("FILEIO_file_3d_img");
		Icn_File_Image = (Texture2D)Resources.Load("FILEIO_file_art_img");
		Icn_File_Movie = (Texture2D)Resources.Load("FILEIO_file_movie_img");
		Icn_File_Audio = (Texture2D)Resources.Load("FILEIO_file_sound_img");
		Icon = Icn_File;
		DialogueBox = new Rect(Screen.width / 2 - DialogueWidth / 2, Screen.height / 2 - DialogueHeight / 2, DialogueWidth, DialogueHeight);
		string fileExt = FileIO.FileExt;
		showext = fileExt.Replace("*", " ");
		showext = showext.Replace(",", " /");
		string[] array = fileExt.Split(',');
		foreach (string text in array)
		{
			extnList.Add(text.ToLower());
			if (FileIO.OS == 1)
			{
				extnList.Add(text.ToUpper());
			}
		}
	}

	private void OnGUI()
	{
		GUI.skin = FileIO_Skin;
		DialogueBox = GUI.Window(0, DialogueBox, Display, "");
	}

	private void Display(int windowID)
	{
		GUILayout.BeginArea(new Rect(10f, 10f, DialogueWidth - 20, DialogueHeight - 20));
		switch (FileIO.GUIno)
		{
		case 0:
			if (FileIO.path == "")
			{
				FileIO.path = FileSelector(FileIO.FileExt);
			}
			else if (File.Exists(FileIO.path))
			{
				FileIO.LoadHandle();
				FileIO.CloseGUI();
			}
			break;
		case 1:
			if (FileIO.path == "")
			{
				FileIO.path = ProjectBrowser();
			}
			else if (File.Exists(FileIO.path))
			{
				FileIO.LoadHandle();
				FileIO.CloseGUI();
			}
			break;
		case 2:
			ProjectCreateSelector();
			break;
		case 3:
			if (FileIO.path == "")
			{
				FileIO.path = ProjectBrowseSelector();
			}
			else if (File.Exists(FileIO.path))
			{
				FileIO.LoadHandle();
				FileIO.CloseGUI();
			}
			break;
		}
		GUILayout.EndArea();
		GUI.DragWindow();
	}

	private string FileSelector(string FileExt)
	{
		GUI.Box(new Rect(0f, 0f, 64f, 64f), Icn_Folder);
		GUI.Label(new Rect(64f, 0f, DialogueWidth - 114, 64f), FileIO.FBTitle);
		if (!drivelevel)
		{
			GUI.TextField(new Rect(50f, 64f, 430f, 32f), "Path: " + FileIO.dI.FullName);
		}
		else
		{
			GUI.TextField(new Rect(50f, 64f, 430f, 32f), "Connected Storage: ");
		}
		if (GUI.Button(new Rect(485f, 64f, 32f, 32f), Icn_ParentFolder))
		{
			if (FileIO.dI.FullName != FileIO.RootDir && !drivelevel)
			{
				FileIO.dI = FileIO.dI.Parent;
			}
			else
			{
				drivelevel = true;
				drives = Directory.GetLogicalDrives();
				validdrives = new List<string>();
				string[] array = drives;
				foreach (string text in array)
				{
					bool flag;
					try
					{
						oldDI = FileIO.dI;
						FileIO.dI = new DirectoryInfo(text);
						FileIO.dI.GetDirectories();
						flag = true;
					}
					catch
					{
						flag = false;
					}
					FileIO.dI = oldDI;
					if (flag)
					{
						validdrives.Add(text);
					}
				}
			}
		}
		GUI.Box(new Rect(50f, 105f, DialogueWidth - 112, DialogueHeight - 176), "", "BrowserFileBox");
		GUILayout.BeginArea(new Rect(60f, 107f, DialogueWidth - 100, DialogueHeight - 179));
		scrollPosition = GUILayout.BeginScrollView(scrollPosition);
		if (!drivelevel)
		{
			DirectoryInfo[] directories = FileIO.dI.GetDirectories();
			foreach (DirectoryInfo directoryInfo in directories)
			{
				if (File.Exists(directoryInfo.FullName + sep + FileIO.projectdatafile))
				{
					if (GUILayout.Button(new GUIContent(directoryInfo.Name, Icn_File_Project), "BrowserItem"))
					{
						FileIO.dI = directoryInfo;
					}
				}
				else if (GUILayout.Button(new GUIContent(directoryInfo.Name, Icn_SmallFolder), "BrowserItem"))
				{
					FileIO.dI = directoryInfo;
				}
			}
			foreach (string extn in extnList)
			{
				GetIcon(extn);
				FileInfo[] files = FileIO.dI.GetFiles(extn);
				foreach (FileInfo fileInfo in files)
				{
					if (GUILayout.Button(new GUIContent(fileInfo.Name, Icon), "BrowserItem"))
					{
						return fileInfo.FullName;
					}
				}
			}
		}
		else
		{
			foreach (string validdrife in validdrives)
			{
				if (GUILayout.Button(new GUIContent(validdrife, Icn_SmallFolder), "BrowserItem"))
				{
					oldDI = FileIO.dI;
					FileIO.dI = new DirectoryInfo(validdrife);
					FileIO.dI.GetDirectories();
					FileIO.RootDir = FileIO.dI.Root.ToString();
					drivelevel = false;
				}
			}
		}
		GUILayout.EndScrollView();
		GUILayout.EndArea();
		GUI.Label(new Rect(50f, DialogueHeight - 60, DialogueWidth - 100, 40f), "Files of type: " + showext, "BrowserExtList");
		if (GUI.Button(new Rect(DialogueWidth - 120, DialogueHeight - 60, 80f, 40f), "Cancel", "BrowserRoundButton"))
		{
			FileIO.CloseGUI();
		}
		return "";
	}

	private string ProjectBrowser()
	{
		GUI.Box(new Rect(60f, 20f, 64f, 64f), Icn_ProjectLoad);
		GUI.Label(new Rect(124f, 20f, DialogueWidth - 114, 64f), FileIO.FBTitle);
		GUI.Box(new Rect(60f, 105f, DialogueWidth - 174, DialogueHeight - 236), "", "BrowserFileBox");
		GUILayout.BeginArea(new Rect(80f, 107f, DialogueWidth - 174, DialogueHeight - 239));
		scrollPosition = GUILayout.BeginScrollView(scrollPosition);
		foreach (string extn in extnList)
		{
			GetIcon(extn);
			foreach (FileIO.Projects currentproject in FileIO.currentprojects)
			{
				if (GUILayout.Button(new GUIContent(currentproject.name, Icn_File_Project), "BrowserItem"))
				{
					return currentproject.projectfolder + sep + FileIO.projectdatafile;
				}
			}
		}
		GUILayout.EndScrollView();
		GUILayout.EndArea();
		if (GUI.Button(new Rect(65f, DialogueHeight - 120, 120f, 40f), "Create New", "BrowserRoundButton"))
		{
			FileIO.GUIno = 2;
		}
		if (GUI.Button(new Rect(185f, DialogueHeight - 120, 120f, 40f), "Find Existing", "BrowserRoundButton"))
		{
			FileIO.GUIno = 3;
		}
		if (FileIO.isProject && GUI.Button(new Rect(DialogueWidth - 120, DialogueHeight - 120, 80f, 40f), "Cancel", "BrowserRoundButton"))
		{
			FileIO.CloseGUI();
		}
		return "";
	}

	private string ProjectCreateSelector()
	{
		GUI.Box(new Rect(0f, 0f, 64f, 64f), Icn_Folder);
		GUI.Label(new Rect(64f, 0f, DialogueWidth - 114, 64f), "Create New Project");
		GUI.Label(new Rect(50f, 54f, DialogueWidth - 50, 40f), "Browse to the directory where the project should be created:", "BrowserExtList");
		if (!drivelevel)
		{
			GUI.TextField(new Rect(50f, 84f, 430f, 32f), "Path: " + FileIO.dI.FullName);
		}
		else
		{
			GUI.TextField(new Rect(50f, 84f, 430f, 32f), "Connected Storage: ");
		}
		if (GUI.Button(new Rect(485f, 84f, 32f, 32f), Icn_ParentFolder))
		{
			if (FileIO.dI.FullName != FileIO.RootDir && !drivelevel)
			{
				FileIO.dI = FileIO.dI.Parent;
			}
			else
			{
				drivelevel = true;
				drives = Directory.GetLogicalDrives();
				validdrives = new List<string>();
				string[] array = drives;
				foreach (string text in array)
				{
					bool flag;
					try
					{
						oldDI = FileIO.dI;
						FileIO.dI = new DirectoryInfo(text);
						FileIO.dI.GetDirectories();
						flag = true;
					}
					catch
					{
						flag = false;
					}
					FileIO.dI = oldDI;
					if (flag)
					{
						validdrives.Add(text);
					}
				}
			}
		}
		GUI.Box(new Rect(50f, 125f, DialogueWidth - 112, DialogueHeight - 236), "", "BrowserFileBox");
		GUILayout.BeginArea(new Rect(60f, 127f, DialogueWidth - 100, DialogueHeight - 239));
		scrollPosition = GUILayout.BeginScrollView(scrollPosition);
		if (!drivelevel)
		{
			DirectoryInfo[] directories = FileIO.dI.GetDirectories();
			foreach (DirectoryInfo directoryInfo in directories)
			{
				if (File.Exists(directoryInfo.FullName + sep + FileIO.projectdatafile))
				{
					GUILayout.Button(new GUIContent(directoryInfo.Name, Icn_File_Project), "BrowserItem");
				}
				else if (GUILayout.Button(new GUIContent(directoryInfo.Name, Icn_SmallFolder), "BrowserItem"))
				{
					FileIO.dI = directoryInfo;
				}
			}
		}
		else
		{
			foreach (string validdrife in validdrives)
			{
				if (GUILayout.Button(new GUIContent(validdrife, Icn_SmallFolder), "BrowserItem"))
				{
					oldDI = FileIO.dI;
					FileIO.dI = new DirectoryInfo(validdrife);
					FileIO.dI.GetDirectories();
					FileIO.RootDir = FileIO.dI.Root.ToString();
					drivelevel = false;
				}
			}
		}
		GUILayout.EndScrollView();
		GUILayout.EndArea();
		GUI.Label(new Rect(50f, 275f, DialogueWidth - 50, 40f), "Enter a unique name for the project:", "BrowserExtList");
		newprojectname = GUI.TextField(new Rect(50f, 305f, 340f, 32f), newprojectname, 100);
		if (GUI.Button(new Rect(DialogueWidth - 200, 305f, 80f, 40f), "Create", "BrowserRoundButton"))
		{
			string text2 = newprojectname.ToLower();
			text2 = text2.Replace(" ", "_");
			err = "";
			if (text2 != "")
			{
				text2 += FileIO.projectfolderamend;
				if (text2.IndexOfAny(" \\ / : * ? \" < > | ' ".ToCharArray()) >= 0)
				{
					err = "Please remove these \\ / : * ? \" < > | ' symbols from project name!";
				}
				else
				{
					string text3 = FileIO.dI.FullName + sep + text2;
					if (Directory.Exists(text3))
					{
						err = "Folder with this project name already exists!";
					}
					else
					{
						FileIO.ProjectCreate(newprojectname, text3);
						FileIO.path = text3 + sep + FileIO.projectdatafile;
						FileIO.LoadHandle();
						FileIO.CloseGUI();
					}
				}
			}
			else
			{
				err = "Please enter a name for this project!";
			}
		}
		if (err != "")
		{
			GUI.Label(new Rect(50f, 335f, DialogueWidth - 50, 40f), err, "BrowserErrorText");
		}
		if (GUI.Button(new Rect(DialogueWidth - 120, 305f, 80f, 40f), "Cancel", "BrowserRoundButton"))
		{
			FileIO.GUIno = 1;
		}
		return "";
	}

	private string ProjectBrowseSelector()
	{
		GUI.Box(new Rect(0f, 0f, 64f, 64f), Icn_Folder);
		GUI.Label(new Rect(64f, 0f, DialogueWidth - 114, 64f), "Find Existing Project");
		if (!drivelevel)
		{
			GUI.TextField(new Rect(50f, 64f, 430f, 32f), "Path: " + FileIO.dI.FullName);
		}
		else
		{
			GUI.TextField(new Rect(50f, 64f, 430f, 32f), "Connected Storage: ");
		}
		if (GUI.Button(new Rect(485f, 64f, 32f, 32f), Icn_ParentFolder))
		{
			if (FileIO.dI.FullName != FileIO.RootDir && !drivelevel)
			{
				FileIO.dI = FileIO.dI.Parent;
			}
			else
			{
				drivelevel = true;
				drives = Directory.GetLogicalDrives();
				validdrives = new List<string>();
				string[] array = drives;
				foreach (string text in array)
				{
					bool flag;
					try
					{
						oldDI = FileIO.dI;
						FileIO.dI = new DirectoryInfo(text);
						FileIO.dI.GetDirectories();
						flag = true;
					}
					catch
					{
						flag = false;
					}
					FileIO.dI = oldDI;
					if (flag)
					{
						validdrives.Add(text);
					}
				}
			}
		}
		GUI.Box(new Rect(50f, 105f, DialogueWidth - 112, DialogueHeight - 176), "", "BrowserFileBox");
		GUILayout.BeginArea(new Rect(60f, 107f, DialogueWidth - 100, DialogueHeight - 179));
		scrollPosition = GUILayout.BeginScrollView(scrollPosition);
		if (!drivelevel)
		{
			DirectoryInfo[] directories = FileIO.dI.GetDirectories();
			foreach (DirectoryInfo directoryInfo in directories)
			{
				if (File.Exists(directoryInfo.FullName + sep + FileIO.projectdatafile))
				{
					if (GUILayout.Button(new GUIContent(directoryInfo.Name, Icn_File_Project), "BrowserItem"))
					{
						return directoryInfo.FullName + sep + FileIO.projectdatafile;
					}
				}
				else if (GUILayout.Button(new GUIContent(directoryInfo.Name, Icn_SmallFolder), "BrowserItem"))
				{
					FileIO.dI = directoryInfo;
				}
			}
		}
		else
		{
			foreach (string validdrife in validdrives)
			{
				if (GUILayout.Button(new GUIContent(validdrife, Icn_SmallFolder), "BrowserItem"))
				{
					oldDI = FileIO.dI;
					FileIO.dI = new DirectoryInfo(validdrife);
					FileIO.dI.GetDirectories();
					FileIO.RootDir = FileIO.dI.Root.ToString();
					drivelevel = false;
				}
			}
		}
		GUILayout.EndScrollView();
		GUILayout.EndArea();
		if (GUI.Button(new Rect(DialogueWidth - 120, DialogueHeight - 60, 80f, 40f), "Cancel", "BrowserRoundButton"))
		{
			FileIO.GUIno = 1;
		}
		return "";
	}

	private void GetIcon(string ext)
	{
		bool flag = false;
		switch (ext.ToLower())
		{
		case "*.obj":
			Icon = Icn_File_3d;
			flag = true;
			break;
		case "*.dae":
			Icon = Icn_File_3d;
			flag = true;
			break;
		case "*.jpg":
			Icon = Icn_File_Image;
			flag = true;
			break;
		case "*.png":
			Icon = Icn_File_Image;
			flag = true;
			break;
		case "*.ogg":
			Icon = Icn_File_Audio;
			flag = true;
			break;
		case "*.mp3":
			Icon = Icn_File_Audio;
			flag = true;
			break;
		case "*.wav":
			Icon = Icn_File_Audio;
			flag = true;
			break;
		case "*.ogv":
			Icon = Icn_File_Movie;
			flag = true;
			break;
		case "*.mp4":
			Icon = Icn_File_Movie;
			flag = true;
			break;
		case "*.wmv":
			Icon = Icn_File_Movie;
			flag = true;
			break;
		}
		if (!flag)
		{
			Icon = Icn_File;
		}
	}
}
