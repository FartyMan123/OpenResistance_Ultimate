//! Setup dialogs for the server admin, built on base-game configurable dialogs (presets in AROR_SetupDialogs.conf).
//! Button handling is overridden without calling super, so each dialog decides itself when to close.

//------------------------------------------------------------------------------------------------
//! Main choice: new or load
class AROR_SetupMainDialog : SCR_ConfigurableDialogUi
{
	protected AROR_SetupUIComponent m_SetupUI;

	//------------------------------------------------------------------------------------------------
	void AROR_SetupMainDialog(AROR_SetupUIComponent setupUI)
	{
		m_SetupUI = setupUI;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnButtonPressed(SCR_InputButtonComponent button)
	{
		string tag = GetButtonTag(button);
		Close();

		if (!m_SetupUI)
			return;

		if (tag == "new")
			m_SetupUI.StartHQPicking();
		else if (tag == "load")
			m_SetupUI.RequestSaveList();
	}
}

//------------------------------------------------------------------------------------------------
//! Confirms the HQ position picked on the map
class AROR_SetupHQConfirmDialog : SCR_ConfigurableDialogUi
{
	protected AROR_SetupUIComponent m_SetupUI;

	//------------------------------------------------------------------------------------------------
	void AROR_SetupHQConfirmDialog(AROR_SetupUIComponent setupUI)
	{
		m_SetupUI = setupUI;
	}

	//------------------------------------------------------------------------------------------------
	void SetPlacementValid(bool valid, string message)
	{
		SetMessage(message);

		SCR_InputButtonComponent startButton = FindButton("start");
		if (startButton)
			startButton.SetEnabled(valid);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnButtonPressed(SCR_InputButtonComponent button)
	{
		string tag = GetButtonTag(button);
		Close();

		if (!m_SetupUI)
			return;

		if (tag == "start")
			m_SetupUI.ConfirmHQ();
		else if (tag == "back")
			m_SetupUI.CancelHQPicking();
		// "repick": just close, the map stays open
	}
}

//------------------------------------------------------------------------------------------------
//! Steps through the saves sent by the server
class AROR_SetupLoadDialog : SCR_ConfigurableDialogUi
{
	protected AROR_SetupUIComponent m_SetupUI;
	protected ref array<string> m_aSaveLabels = {};
	protected int m_iIndex;

	//------------------------------------------------------------------------------------------------
	void AROR_SetupLoadDialog(AROR_SetupUIComponent setupUI, notnull array<string> saveLabels)
	{
		m_SetupUI = setupUI;
		m_aSaveLabels.Copy(saveLabels);
	}

	//------------------------------------------------------------------------------------------------
	void Refresh()
	{
		int count = m_aSaveLabels.Count();
		if (count == 0)
		{
			SetMessage("No saved campaigns for this map.");
		}
		else
		{
			SetMessage(string.Format("Save %1 of %2:\n%3", m_iIndex + 1, count, m_aSaveLabels[m_iIndex]));
		}

		SetButtonEnabled("previous", m_iIndex > 0);
		SetButtonEnabled("next", m_iIndex < count - 1);
		SetButtonEnabled("load", count > 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetButtonEnabled(string tag, bool enabled)
	{
		SCR_InputButtonComponent button = FindButton(tag);
		if (button)
			button.SetEnabled(enabled);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnButtonPressed(SCR_InputButtonComponent button)
	{
		string tag = GetButtonTag(button);

		if (tag == "previous" || tag == "next")
		{
			if (tag == "previous" && m_iIndex > 0)
				m_iIndex--;
			else if (tag == "next" && m_iIndex < m_aSaveLabels.Count() - 1)
				m_iIndex++;

			Refresh();
			return;
		}

		Close();

		if (!m_SetupUI)
			return;

		if (tag == "load")
			m_SetupUI.RequestLoad(m_iIndex);
		else if (tag == "back")
			m_SetupUI.OpenMainDialog();
	}
}
