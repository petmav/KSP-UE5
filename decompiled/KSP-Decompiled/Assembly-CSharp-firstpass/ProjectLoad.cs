using System.Collections;
using System.IO;
using System.Xml;
using UnityEngine;

public class ProjectLoad : MonoBehaviour
{
	public string PrjPath;

	private XmlDocument xdoc;

	private string basepath;

	private string ProjectName;

	private void Start()
	{
		if (PrjPath != "")
		{
			if (!PrjPath.Contains("http://"))
			{
				basepath = Path.GetDirectoryName(PrjPath);
				PrjPath = "file://" + PrjPath;
			}
			else
			{
				basepath = ((PrjPath.IndexOf("/") == -1) ? "" : PrjPath.Substring(0, PrjPath.LastIndexOf("/")));
			}
			StartCoroutine(Load(PrjPath));
		}
		else
		{
			FileIO.isProject = false;
			FileIO.Loaded(success: false);
			Object.Destroy(this);
		}
	}

	public IEnumerator Load(string path)
	{
		WWW loader = new WWW(path);
		yield return loader;
		if (loader.error != null)
		{
			Debug.Log("File Not Found");
		}
		else
		{
			ReadProjectData(loader.text);
		}
		FileIO.isProject = true;
		FileIO.CurProjectFolder = basepath;
		FileIO.CurProjectName = ProjectName;
		base.gameObject.name = "Project:" + ProjectName;
		FileIO.ProjectRegister();
		FileIO.Loaded(success: true);
		Object.Destroy(this);
	}

	private void ReadProjectData(string prjdata)
	{
		xdoc = new XmlDocument();
		xdoc.LoadXml(prjdata);
		XmlElement xmlElement = ((XmlElement)xdoc.GetElementsByTagName("UnityProject")[0]) as XmlElement;
		ProjectName = xmlElement["Project_Name"].InnerXml;
	}
}
