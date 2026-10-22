//! Setup dialogs for the server admin, built on base-game configurable dialogs (presets in AROR_SetupDialogs.conf).
//! A dialog object is a widget handler owned by the dialog's widgets, so it is destroyed with them.
//! A pressed button records its tag and closes the dialog. When the dialog has closed, the action is queued on
//! AROR_SetupUIComponent for the next frame: opening another menu while the dialog is still closing either leaves
//! the dialog on top (dialogs draw above menus) or breaks the menu manager's close sequence.

//------------------------------------------------------------------------------------------------
class AROR_SetupDialogBase : SCR_ConfigurableDialogUi
{
	protected AROR_SetupUIComponent m_SetupUI;
	protected string m_sPressedTag;

	//------------------------------------------------------------------------------------------------
	//! Identifies the dialog in AROR_SetupUIComponent.HandleDialogAction
	protected string GetDialogId();

	//------------------------------------------------------------------------------------------------
	//! Extra value passed with the action (e.g. the selected save index)
	protected int GetActionValue()
	{
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnButtonPressed(SCR_InputButtonComponent button)
	{
		super.OnButtonPressed(button);

		// Ignore further clicks while closing
		if (!m_sPressedTag.IsEmpty())
			return;

		m_sPressedTag = GetButtonTag(button);
		Close();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		super.OnMenuClose();

		if (m_SetupUI && !m_sPressedTag.IsEmpty())
			m_SetupUI.QueueDialogAction(GetDialogId(), m_sPressedTag, GetActionValue());
	}
}

//------------------------------------------------------------------------------------------------
//! Main choice: new or load
class AROR_SetupMainDialog : AROR_SetupDialogBase
{
	//------------------------------------------------------------------------------------------------
	void AROR_SetupMainDialog(AROR_SetupUIComponent setupUI)
	{
		m_SetupUI = setupUI;
	}

	//------------------------------------------------------------------------------------------------
	override protected string GetDialogId()
	{
		return AROR_SetupUIComponent.DIALOG_MAIN;
	}
}

//------------------------------------------------------------------------------------------------
//! Confirms the HQ position picked on the map
class AROR_SetupHQConfirmDialog : AROR_SetupDialogBase
{
	//------------------------------------------------------------------------------------------------
	void AROR_SetupHQConfirmDialog(AROR_SetupUIComponent setupUI)
	{
		m_SetupUI = setupUI;
	}

	//------------------------------------------------------------------------------------------------
	override protected string GetDialogId()
	{
		return AROR_SetupUIComponent.DIALOG_HQ_CONFIRM;
	}

	//------------------------------------------------------------------------------------------------
	void SetPlacementValid(bool valid, string message)
	{
		SetMessage(message);

		SCR_InputButtonComponent startButton = FindButton("start");
		if (startButton)
			startButton.SetEnabled(valid);
	}
}

//------------------------------------------------------------------------------------------------
//! Steps through the saves sent by the server
class AROR_SetupLoadDialog : AROR_SetupDialogBase
{
	protected ref array<string> m_aSaveLabels = {};
	protected int m_iIndex;

	//------------------------------------------------------------------------------------------------
	void AROR_SetupLoadDialog(AROR_SetupUIComponent setupUI, notnull array<string> saveLabels)
	{
		m_SetupUI = setupUI;
		m_aSaveLabels.Copy(saveLabels);
	}

	//------------------------------------------------------------------------------------------------
	override protected string GetDialogId()
	{
		return AROR_SetupUIComponent.DIALOG_LOAD;
	}

	//------------------------------------------------------------------------------------------------
	override protected int GetActionValue()
	{
		return m_iIndex;
	}

	//------------------------------------------------------------------------------------------------
	void Refresh()
	{
		int count = m_aSaveLabels.Count();
		if (count == 0)
			SetMessage("No saved campaigns for this map.");
		else
			SetMessage(string.Format("Save %1 of %2:\n%3", m_iIndex + 1, count, m_aSaveLabels[m_iIndex]));

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

		// Paging keeps the dialog open
		if (tag == "previous" || tag == "next")
		{
			if (tag == "previous" && m_iIndex > 0)
				m_iIndex--;
			else if (tag == "next" && m_iIndex < m_aSaveLabels.Count() - 1)
				m_iIndex++;

			Refresh();
			return;
		}

		super.OnButtonPressed(button);
	}
}
