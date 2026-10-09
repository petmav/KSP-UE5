using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using UnityEngine;

public class FileIO : MonoBehaviour
{
	public delegate void LoadCallback(GameObject[] loaded);

	public class Projects
	{
		public string name;

		public string projectfolder;
	}

	public static bool LOCKMOUSE = false;

	public static GameObject[] LastLoaded;

	private static LoadCallback _CurLoadCallBack;

	private static bool isCallback = false;

	public static string PFolder = "";

	public static string AppDataFolder = "";

	public static string RootDir = "/";

	public static int OS = 0;

	private static string projectlistfile = "CurrentProjects.log";

	public static string projectfolderamend = "_(XProject)";

	public static string projectdatafile = "ProjectData.xml";

	public static string projectmeshfolder = "Meshes";

	public static string projectmeshext = ".mesh";

	public static string projecttexturefolder = "Textures";

	public static GameObject ProjectGO;

	public static string CurProjectFolder = "";

	public static string CurProjectName = "";

	public static bool isProject = false;

	public static List<Projects> currentprojects;

	public static string FileExt = "";

	public static string FBTitle = "";

	public static string path = "";

	public static string location = "";

	public static DirectoryInfo dI = null;

	public static int GUIno = 0;

	public static string startFolder = "";

	private static bool MakeCollider = false;

	private static bool SaveToProject = false;

	private static bool Init = false;

	private static bool GUIisActive = false;

	private static bool LSHisActive = false;

	private static GameObject GuiGO;

	public static bool ignoreMaterials;

	public static void ProjectsMenu()
	{
		if (!GUIisActive)
		{
			InitPlatform();
			if (OS != 3)
			{
				ProjectsRead();
				GUIno = 1;
				FileExt = "";
				FBTitle = "Open Existing or Create New Project";
				InitGUI();
			}
		}
	}

	private static void ProjectsRead()
	{
		currentprojects = new List<Projects>();
		string text = Path.DirectorySeparatorChar.ToString();
		try
		{
			using (StreamReader streamReader = new StreamReader(AppDataFolder + text + projectlistfile))
			{
				string input;
				while ((input = streamReader.ReadLine()) != null)
				{
					string[] array = Regex.Split(input, "::");
					string filepath = array[1] + text + projectdatafile;
					string text2 = "";
					if (!Directory.Exists(array[1]))
					{
						text2 = ResolvePath(filepath, location, wwwload: false);
						if (text2 != "")
						{
							text2 = Path.GetDirectoryName(text2);
						}
					}
					else
					{
						text2 = array[1];
					}
					if (!(text2 != ""))
					{
						continue;
					}
					Projects projects = new Projects();
					projects.name = array[0];
					projects.projectfolder = text2;
					bool flag = false;
					foreach (Projects currentproject in currentprojects)
					{
						if (currentproject.name == projects.name && currentproject.projectfolder == projects.projectfolder)
						{
							flag = true;
						}
					}
					if (!flag)
					{
						currentprojects.Add(projects);
					}
				}
				streamReader.Close();
			}
			using (StreamWriter streamWriter = new StreamWriter(AppDataFolder + text + projectlistfile))
			{
				foreach (Projects currentproject2 in currentprojects)
				{
					streamWriter.WriteLine(currentproject2.name + "::" + currentproject2.projectfolder);
				}
				streamWriter.Close();
			}
		}
		catch
		{
			Debug.Log("The assets/projectlistfile could not be read > Possibly First visit");
		}
	}

	public static void ProjectRegister()
	{
		if (currentprojects == null)
		{
			ProjectsRead();
		}
		string text = Path.DirectorySeparatorChar.ToString();
		Projects projects = new Projects();
		projects.name = CurProjectName;
		projects.projectfolder = CurProjectFolder;
		bool flag = false;
		foreach (Projects currentproject in currentprojects)
		{
			if (currentproject.name == projects.name && currentproject.projectfolder == projects.projectfolder)
			{
				flag = true;
			}
		}
		if (!flag)
		{
			currentprojects.Add(projects);
		}
		try
		{
			using (StreamWriter streamWriter = new StreamWriter(AppDataFolder + text + projectlistfile))
			{
				foreach (Projects currentproject2 in currentprojects)
				{
					streamWriter.WriteLine(currentproject2.name + "::" + currentproject2.projectfolder);
				}
				streamWriter.Close();
			}
		}
		catch
		{
			Debug.Log("error writing project to projectlist");
		}
	}

	public static bool ProjectCreate(string name, string path)
	{
		bool result = false;
		if (!Directory.Exists(path))
		{
			try
			{
				Directory.CreateDirectory(path);
				using (StreamWriter streamWriter = new StreamWriter(path + Path.DirectorySeparatorChar + projectdatafile))
				{
					streamWriter.WriteLine("<UnityProject>");
					streamWriter.WriteLine("<Project_Name>" + name + "</Project_Name>");
					streamWriter.WriteLine(string.Concat("<Project_Created>", DateTime.Now, "</Project_Created>"));
					streamWriter.WriteLine("</UnityProject>");
					streamWriter.Close();
				}
				using (StreamWriter streamWriter2 = new StreamWriter(AppDataFolder + Path.DirectorySeparatorChar + projectlistfile, append: true))
				{
					streamWriter2.WriteLine(name + "::" + path);
					streamWriter2.Close();
				}
				result = true;
			}
			catch
			{
				Debug.Log("error either creating directory/writing project file/writing to assets/projectlist file");
				result = false;
			}
		}
		return result;
	}

	public static bool ProjectSilentCreate(string name, string path, bool replace)
	{
		bool flag = false;
		InitPlatform();
		if (OS != 3)
		{
			string text = name.ToLower();
			text = text.Replace(" ", "_");
			string replacement = " ";
			text = Regex.Replace(new Regex("[\\*/:<>?|\"-]").Replace(text, replacement), "\\s+", " ");
			string text2 = Path.DirectorySeparatorChar.ToString();
			if (!replace)
			{
				text = GenerateFolderName(path + text2, text, projectfolderamend);
			}
			flag = ProjectCreate(name, path + text2 + text + projectfolderamend);
			if (flag)
			{
				FileIO.path = path + text2 + text + projectfolderamend + text2 + projectdatafile;
				LoadHandle();
			}
			else
			{
				Debug.Log("Something went wrong in projectsilentcreate FileIO");
			}
		}
		return flag;
	}

	public static bool ProjectSilentLoad(string name, string path)
	{
		bool result = false;
		InitPlatform();
		if (OS != 3)
		{
			string text = name.ToLower();
			text = text.Replace(" ", "_");
			string replacement = " ";
			text = Regex.Replace(new Regex("[\\*/:<>?|\"-]").Replace(text, replacement), "\\s+", " ");
			string text2 = Path.DirectorySeparatorChar.ToString();
			if (currentprojects == null)
			{
				ProjectsRead();
			}
			bool flag = false;
			foreach (Projects currentproject in currentprojects)
			{
				if (currentproject.name == name && currentproject.projectfolder == path + text2 + text + projectfolderamend)
				{
					flag = true;
				}
			}
			if (flag)
			{
				FileIO.path = path + text2 + text + projectfolderamend + text2 + projectdatafile;
				LoadHandle();
				result = true;
			}
			else
			{
				result = ProjectSilentCreate(name, path, replace: true);
			}
		}
		return result;
	}

	public static void SaveMeshesToProject(GameObject[] gos, bool replace)
	{
		string text = Path.DirectorySeparatorChar.ToString();
		if (!Directory.Exists(CurProjectFolder + text + projectmeshfolder))
		{
			Directory.CreateDirectory(CurProjectFolder + text + projectmeshfolder);
		}
		if (!Directory.Exists(CurProjectFolder + text + projecttexturefolder))
		{
			Directory.CreateDirectory(CurProjectFolder + text + projecttexturefolder);
		}
		foreach (GameObject gameObject in gos)
		{
			bool flag = false;
			Mesh mesh = gameObject.GetComponent<MeshFilter>().mesh;
			if (gameObject.GetComponent<MeshCollider>() != null)
			{
				flag = !(gameObject.GetComponent<MeshCollider>().sharedMesh.name == mesh.name);
			}
			string text2 = mesh.name;
			if (!replace)
			{
				text2 = GenerateFileName(CurProjectFolder + text + projectmeshfolder + text, text2, projectmeshext);
			}
			ProjectModelSave.WriteMesh(mesh, CurProjectFolder + text + projectmeshfolder + text + text2 + projectmeshext);
			gameObject.GetComponent<MeshFilter>().mesh.name = text2;
			if (flag)
			{
				Mesh sharedMesh = gameObject.GetComponent<MeshCollider>().sharedMesh;
				text2 = sharedMesh.name;
				if (!replace)
				{
					text2 = GenerateFileName(CurProjectFolder + text + projectmeshfolder + text, text2, projectmeshext);
				}
				ProjectModelSave.WriteMesh(sharedMesh, CurProjectFolder + text + projectmeshfolder + text + text2 + projectmeshext);
				gameObject.GetComponent<MeshCollider>().sharedMesh.name = text2;
			}
		}
	}

	public static void SaveTextureToProject(Texture2D ct, bool replace)
	{
		SaveTexturesToProject(new Texture2D[1] { ct }, replace);
	}

	public static void SaveTexturesToProject(Texture2D[] textures, bool replace)
	{
		string text = Path.DirectorySeparatorChar.ToString();
		if (!Directory.Exists(CurProjectFolder + text + projecttexturefolder))
		{
			Directory.CreateDirectory(CurProjectFolder + text + projecttexturefolder);
		}
		foreach (Texture2D obj in textures)
		{
			string text2 = obj.name;
			if (!replace)
			{
				text2 = GenerateFileName(CurProjectFolder + text + projecttexturefolder + text, text2, ".png");
			}
			ProjectModelSave.WriteTexture(obj, CurProjectFolder + text + projecttexturefolder + text + text2 + ".png");
		}
	}

	public static void Load3d()
	{
		if (!GUIisActive)
		{
			InitPlatform();
			if (path == "")
			{
				GUIno = 0;
				FileExt = "*.obj,*.dae";
				FBTitle = "Import 3D Object / Mesh";
				InitGUI();
			}
			else
			{
				LoadHandle();
				path = "";
			}
		}
	}

	public static void Load3d(string _path)
	{
		path = _path;
		Load3d();
	}

	public static void Load3d(bool _makecol)
	{
		MakeCollider = _makecol;
		Load3d();
	}

	public static void Load3d(bool _makecol, bool _save)
	{
		MakeCollider = _makecol;
		SaveToProject = CheckSave(_save);
		Load3d();
	}

	public static void Load3d(bool _makecol, bool _save, LoadCallback usercallback)
	{
		_CurLoadCallBack = usercallback;
		isCallback = true;
		MakeCollider = _makecol;
		SaveToProject = CheckSave(_save);
		Load3d();
	}

	public static void Load3d(string _path, bool _makecol, bool _save)
	{
		path = _path;
		MakeCollider = _makecol;
		SaveToProject = CheckSave(_save);
		Load3d();
	}

	public static void Load3d(string _path, bool _makecol, bool _save, LoadCallback usercallback)
	{
		_CurLoadCallBack = usercallback;
		isCallback = true;
		path = _path;
		MakeCollider = _makecol;
		SaveToProject = CheckSave(_save);
		Load3d();
	}

	public static void LoadHandle()
	{
		if (LSHisActive)
		{
			return;
		}
		LSHisActive = true;
		bool flag = false;
		switch (Path.GetExtension(path).ToLower())
		{
		case ".obj":
		{
			GameObject gameObject = new GameObject(Path.GetFileNameWithoutExtension(path));
			if (isProject)
			{
				gameObject.transform.parent = ProjectGO.transform;
			}
			gameObject.AddComponent<OBJ>();
			OBJ component = gameObject.GetComponent<OBJ>();
			component.objPath = path;
			component.MakeCollider = MakeCollider;
			component.SaveToProject = SaveToProject;
			flag = true;
			break;
		}
		case ".dae":
		{
			GameObject gameObject = new GameObject(Path.GetFileNameWithoutExtension(path));
			if (isProject)
			{
				gameObject.transform.parent = ProjectGO.transform;
			}
			gameObject.AddComponent<DAE>();
			DAE component2 = gameObject.GetComponent<DAE>();
			component2.objPath = path;
			component2.MakeCollider = MakeCollider;
			component2.SaveToProject = SaveToProject;
			flag = true;
			break;
		}
		case ".xml":
			if (Path.GetFileName(path) == projectdatafile)
			{
				if (isProject)
				{
					UnityEngine.Object.Destroy(ProjectGO);
					ProjectGO = new GameObject("Project:Loading");
				}
				else
				{
					ProjectGO = new GameObject("Project:Loading");
				}
				ProjectGO.AddComponent<ProjectLoad>();
				ProjectGO.GetComponent<ProjectLoad>().PrjPath = path;
				flag = true;
			}
			else
			{
				flag = false;
			}
			break;
		case ".jpg":
			flag = false;
			break;
		case ".png":
			flag = false;
			break;
		case ".ogg":
			flag = false;
			break;
		case ".mp3":
			flag = false;
			break;
		case ".wav":
			flag = false;
			break;
		case ".ogv":
			flag = false;
			break;
		case ".mp4":
			flag = false;
			break;
		case ".wmv":
			flag = false;
			break;
		}
		if (!flag)
		{
			Debug.Log("A Load Handler for this file type does not yet exist");
			LSHisActive = false;
		}
	}

	public static void Loaded(bool success)
	{
		LSHisActive = false;
		MakeCollider = false;
		SaveToProject = false;
		if (success && isCallback)
		{
			_CurLoadCallBack(LastLoaded);
		}
	}

	public static void CloseGUI()
	{
		if (GUIisActive)
		{
			UnityEngine.Object.Destroy(GuiGO);
			GUIisActive = false;
			if (!LSHisActive)
			{
				path = "";
				MakeCollider = false;
				SaveToProject = false;
			}
			LOCKMOUSE = false;
		}
	}

	public static string ResolvePath(string filepath, string basepath, bool wwwload)
	{
		string text = "";
		if (OS == 3 || basepath.Contains("http://"))
		{
			text = basepath + filepath;
		}
		else
		{
			filepath = filepath.Replace("file://", "");
			filepath = filepath.Replace("file:\\", "");
			basepath = basepath.Replace("file://", "");
			basepath = basepath.Replace("file:\\", "");
			string text2 = Path.DirectorySeparatorChar.ToString();
			string oldValue = ((!(text2 == "\\")) ? "\\" : "/");
			filepath = filepath.Replace(oldValue, text2);
			if (File.Exists(basepath + filepath))
			{
				text = basepath + filepath;
			}
			else if (File.Exists(basepath + Path.DirectorySeparatorChar + filepath))
			{
				text = basepath + Path.DirectorySeparatorChar + filepath;
			}
			else if (File.Exists(filepath))
			{
				text = filepath;
			}
			else
			{
				string[] array = filepath.Split(text2.ToCharArray());
				string[] array2 = basepath.Split(text2.ToCharArray());
				bool flag = false;
				for (int num = array2.Length - 1; num >= 0; num--)
				{
					for (int num2 = array.Length - 1; num2 >= 0; num2--)
					{
						if (!flag)
						{
							StringBuilder stringBuilder = new StringBuilder();
							for (int i = 0; i < num; i++)
							{
								stringBuilder.Append(array2[i]).Append(text2);
							}
							for (int j = num2; j < array.Length; j++)
							{
								stringBuilder.Append(array[j]).Append(text2);
							}
							stringBuilder.Remove(stringBuilder.Length - 1, 1);
							if (File.Exists(stringBuilder.ToString()))
							{
								flag = true;
								text = stringBuilder.ToString();
								break;
							}
						}
					}
					if (flag)
					{
						break;
					}
				}
			}
			if (wwwload)
			{
				text = "file://" + text;
			}
		}
		return text;
	}

	private static bool CheckSave(bool _save)
	{
		if (_save)
		{
			if (OS == 3)
			{
				return false;
			}
			if (!isProject)
			{
				return false;
			}
			return true;
		}
		return false;
	}

	private static string GenerateFolderName(string basefolder, string foldername, string amend)
	{
		string text = foldername;
		int num = 1;
		bool flag = true;
		while (flag)
		{
			if (Directory.Exists(basefolder + text + amend))
			{
				text = foldername + "_" + num;
				num++;
			}
			else
			{
				flag = false;
			}
		}
		return text;
	}

	private static string GenerateFileName(string basefolder, string origname, string ext)
	{
		string text = origname;
		int num = 1;
		bool flag = true;
		while (flag)
		{
			if (File.Exists(basefolder + text + ext))
			{
				text = origname + "_" + num;
				num++;
			}
			else
			{
				flag = false;
			}
		}
		return text;
	}

	private static void InitGUI()
	{
		if (!GUIisActive)
		{
			GuiGO = new GameObject("BrowserGUI");
			GuiGO.AddComponent<FileGUI>();
			GUIisActive = true;
			LOCKMOUSE = true;
		}
	}

	private static void InitPlatform()
	{
		if (Init)
		{
			return;
		}
		AppDataFolder = Application.dataPath;
		switch (Application.platform)
		{
		case RuntimePlatform.WindowsPlayer:
			OS = 0;
			break;
		case RuntimePlatform.WindowsEditor:
			OS = 0;
			break;
		case RuntimePlatform.OSXPlayer:
			OS = 1;
			break;
		case RuntimePlatform.OSXEditor:
			OS = 1;
			break;
		case RuntimePlatform.IPhonePlayer:
			OS = 2;
			break;
		case RuntimePlatform.Android:
			OS = 2;
			break;
		}
		if (OS < 3 && startFolder == "")
		{
			try
			{
				PFolder = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
				RootDir = Path.GetPathRoot(PFolder);
				location = PFolder;
			}
			catch
			{
				PFolder = "";
				RootDir = "/";
				location = AppDataFolder;
			}
		}
		else
		{
			PFolder = "";
			RootDir = "/";
			location = ((startFolder == "") ? AppDataFolder : startFolder);
		}
		dI = new DirectoryInfo(location);
		Init = true;
	}
}
