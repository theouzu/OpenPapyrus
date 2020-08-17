// V_CMDP.CPP
// Copyright (c) A.Starodub 2006, 2007, 2008, 2009, 2011, 2012, 2013, 2014, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
// Редактирование списка команд
//
#include <pp.h>
#pragma hdrstop

int SLAPI MenuResToMenu(uint resMenuID, PPCommandFolder * pMenu);
//
//
//
#define USEDEFICON         0x01L
#define DEFAULT_MENUS_OFFS 100000L
//
//
//
int SLAPI EditCmdItem(const PPCommandGroup * pGrp, PPCommand * pData, int isDekstopCmd)
{
	class CmdItemDialog : public TDialog {
		DECL_DIALOG_DATA(PPCommand);
		enum {
			ctlgroupFbg = 1
		};
	public:
		CmdItemDialog(const PPCommandGroup * pGrp, int isDesktopCmd) : TDialog(DLG_CMDITEM), IsDesktopCmd(isDesktopCmd), P_Grp(pGrp)
		{
			CmdDescr.GetResourceList(1, CmdSymbList);
			if(IsDesktopCmd)
				FileBrowseCtrlGroup::Setup(this, CTLBRW_CMDITEM_ICON, CTL_CMDITEM_ICON, ctlgroupFbg, PPTXT_SELCMDICON, PPTXT_FILPAT_ICONS, FileBrowseCtrlGroup::fbcgfFile);
			disableCtrl(CTL_CMDITEM_ICON, 1);
			disableCtrl(CTL_CMDITEM_USEDEFICON, !IsDesktopCmd);
			disableCtrl(CTLBRW_CMDITEM_ICON, !IsDesktopCmd);
			enableCommand(cmCmdParam,   IsDesktopCmd);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			StrAssocArray cmd_txt_list;
			RVALUEPTR(Data, pData);
			setCtrlString(CTL_CMDITEM_NAME, Data.Name);
			setCtrlLong(CTL_CMDITEM_ID, Data.ID);
			CmdDescr.GetResourceList(1, cmd_txt_list);
			uint   pos = 0;
			cmd_txt_list.SortByText();
			SetupStrAssocCombo(this, CTLSEL_CMDITEM_CMD, &cmd_txt_list, Data.CmdID, 0);
			SetupWordSelector(CTLSEL_CMDITEM_CMD, 0, Data.CmdID, 2, WordSel_ExtraBlock::fAlwaysSearchBySubStr); // @v10.7.8
			setCtrlString(CTL_CMDITEM_ICON, Data.Icon);
			AddClusterAssoc(CTL_CMDITEM_USEDEFICON, 0, USEDEFICON);
			SetClusterData(CTL_CMDITEM_USEDEFICON, Data.Icon.ToLong() || !Data.Icon.Len());
			AddClusterAssoc(CTL_CMDITEM_FLAGS, 0, PPCommand::fAllowEditFilt);
			SetClusterData(CTL_CMDITEM_FLAGS, Data.Flags);
			disableCtrl(CTLBRW_CMDITEM_ICON, Data.Icon.ToLong() || !Data.Icon.Len());
			disableCtrl(CTLSEL_CMDITEM_CMD, Data.CmdID);
			disableCtrl(CTL_CMDITEM_ID, 1);
			if(Data.ID && CmdDescr.LoadResource(Data.CmdID) > 0)
				enableCommand(cmCmdParam, !(CmdDescr.Flags & PPCommandDescr::fNoParam));
			else
				enableCommand(cmCmdParam, 0);
			{
				FileBrowseCtrlGroup * p_fbg = static_cast<FileBrowseCtrlGroup *>(getGroup(ctlgroupFbg));
				if(p_fbg) {
					SString spath;
					if(!Data.Icon.Len() || Data.Icon.ToLong()) {
						PPGetPath(PPPATH_BIN, spath);
						spath.SetLastSlash().Cat("Icons").SetLastSlash();
					}
					else
						spath = Data.Icon;
					p_fbg->setInitPath(spath);
				}
			}
			SetupCtrls();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			PPID   cmd_id = 0;
			PPCommandDescr cmd_descr;
			getCtrlString(CTL_CMDITEM_NAME, Data.Name);
			THROW_PP(Data.Name.NotEmptyS(), PPERR_NAMENEEDED);
			getCtrlData(CTLSEL_CMDITEM_CMD, &cmd_id);
			getCtrlString(CTL_CMDITEM_ICON, Data.Icon);
			THROW_PP(CmdSymbList.Search(cmd_id), PPERR_INVJOBCMD);
			THROW(cmd_descr.LoadResource(cmd_id));
			Data.CmdID  = cmd_id;
			Data.Flags = 0;
			GetClusterData(CTL_CMDITEM_FLAGS, &Data.Flags);
			if(!Data.Icon.Len())
				Data.Icon.Cat(cmd_descr.IconId);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERR
			return ok;
		}
	private:
		struct _CmdID {
			long   CmdID;
			PPIDArray Ary;
		};
		static int _GetIdListByCommand(const PPCommandItem * pItem, long parentID, void * extraPtr)
		{
			if(pItem) {
				_CmdID * p_e = static_cast<_CmdID *>(extraPtr);
				PPCommand * p_cmd = (pItem->Kind == PPCommandItem::kCommand) ? static_cast<PPCommand *>(pItem->Dup()) : 0;
				if(p_cmd && p_cmd->CmdID == p_e->CmdID)
					p_e->Ary.addUnique(p_cmd->ID);
				ZDELETE(p_cmd);
			}
			return 1;
		}
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_CMDITEM_CMD)) {
				PPID   cmd_id = getCtrlLong(CTLSEL_CMDITEM_CMD);
				if(cmd_id && CmdDescr.LoadResource(cmd_id) > 0) {
					SString name = CmdDescr.Text;
					if(P_Grp) {
						_CmdID _e;
						_e.CmdID = cmd_id;
						P_Grp->Enumerate(_GetIdListByCommand, 0, &_e);
						if(_e.Ary.getCount())
							name.Space().CatChar('#').Cat(_e.Ary.getCount());
					}
					setCtrlString(CTL_CMDITEM_NAME, name);
					if(IsDesktopCmd)
						enableCommand(cmCmdParam, !(CmdDescr.Flags & PPCommandDescr::fNoParam));
				}
				else
					enableCommand(cmCmdParam, 0);
			}
			else if(event.isCmd(cmCmdParam)) {
				uint   sav_offs = Data.Param.GetRdOffs();
				if(CmdDescr.EditCommandParam(getCtrlLong(CTLSEL_JOBITEM_CMD), Data.ID, &Data.Param, 0)) {
					Data.Param.SetRdOffs(sav_offs);
					SetupCtrls();
				}
				else
					PPError();
			}
			else if(event.isCmd(cmClearFilt)) {
				Data.Param.Z();
				SetupCtrls();
			}
			else if(event.isClusterClk(CTL_CMDITEM_USEDEFICON)) {
				long   f = 0;
				GetClusterData(CTL_CMDITEM_USEDEFICON, &f);
				disableCtrl(CTLBRW_CMDITEM_ICON, (f & USEDEFICON));
				if(f & USEDEFICON) {
					long   cmd_id = getCtrlLong(CTLSEL_CMDITEM_CMD);
					PPCommandDescr cmd_descr;
					if(cmd_id && CmdSymbList.Search(cmd_id) > 0 && cmd_descr.LoadResource(cmd_id) > 0) {
						SString icon;
						icon.Cat(cmd_descr.IconId);
						setCtrlString(CTL_CMDITEM_ICON, icon);
					}
				}
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupCtrls()
		{
			showCtrl(CTL_CMDITEM_CLEARFILT, BIN(Data.Param.GetAvailableSize()));
		}

		int    IsDesktopCmd;
		PPCommandDescr CmdDescr;
		const PPCommandGroup * P_Grp;
		StrAssocArray CmdSymbList;
	};
	DIALOG_PROC_BODY_P2(CmdItemDialog, pGrp, isDekstopCmd, pData);
}
//
// CommandsDialog
//
class CommandsDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPCommandGroup); // @v10.7.6 PPCommandFolder-->PPCommandGroup
public:
	explicit CommandsDialog(/*PPCommandGroup * pMenus,*/int isDesktop) : PPListDialog(DLG_LBXSELT, CTL_LBXSEL_LIST), /*P_Menus_(pMenus),*/ IsDesktop(isDesktop)
	{
		SString title;
		setTitle(PPLoadTextS(PPTXT_EDITCMDLIST, title));
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	virtual int setupList()
	{
		int    ok = -1;
		StrAssocArray * p_list = 0;
		StdTreeListBoxDef * p_def = 0;
		if(P_Box) {
			THROW_MEM(p_list = new StrAssocArray);
			THROW(Data.GetCommandList(p_list, 0));
			THROW_MEM(p_def = new StdTreeListBoxDef(p_list, lbtDisposeData|lbtDblClkNotify, 0));
			P_Box->setDef(p_def);
			P_Box->Draw_();
			ok = 1;
		}
		CATCH
			if(p_def)
				delete p_def;
			else
				delete p_list;
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int moveItem(long pos, long id, int up);
	PPCommandItem * GetItem(long id, long * pPos, PPCommandFolder ** pFolder);

	const int IsDesktop;
	//const PPCommandGroup * P_Menus_;
};

int CommandsDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	long   parent_id = 0;
	long   parent_id2 = 0;
	long   v = 1;
	const  PPCommandItem * p_selitem = 0;
	TDialog * p_dlg = new TDialog(DLG_ADDCMD);
	StrAssocArray cmd_list;
	THROW(Data.GetCommandList(&cmd_list, 1));
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->AddClusterAssocDef(CTL_ADDCMD_WHAT,  0, 1);
	p_dlg->AddClusterAssoc(CTL_ADDCMD_WHAT,  1, 2);
	p_dlg->AddClusterAssoc(CTL_ADDCMD_WHAT,  2, 3);
	p_dlg->SetClusterData(CTL_ADDCMD_WHAT, v);
	P_Box->getCurID(&parent_id);
	if(!(p_selitem = Data.SearchByIDRecursive_Const(parent_id, &parent_id2)) || p_selitem->Kind != PPCommandItem::kFolder)
		parent_id = parent_id2;
    //SetupSTreeComboBox(p_dlg, CTLSEL_ADDCMD_PARENT, &cmd_list, parent_id, 0);
	SetupStrAssocCombo(p_dlg, CTLSEL_ADDCMD_PARENT, &cmd_list, parent_id, 0, 0);
	if(ExecView(p_dlg) == cmOK) {
		PPCommandItem * p_item = 0;
		PPCommandItem new_sep;
		PPCommand new_cmd;
		PPCommandFolder new_cmdfolder;
		p_dlg->GetClusterData(CTL_ADDCMD_WHAT, &v);
		p_dlg->getCtrlData(CTLSEL_ADDCMD_PARENT, &parent_id);
		if(v == 1) {
			if(EditCmdItem(&Data, &new_cmd, IsDesktop) > 0) // @v10.8.5 P_Menus_-->&Data
				p_item = static_cast<PPCommandItem *>(&new_cmd);
		}
		else if(v == 2) {
			if(EditName(new_cmdfolder.Name) > 0)
				p_item = static_cast<PPCommandItem *>(&new_cmdfolder);
		}
		else {
			new_sep.Name.Z().CatCharN('-', 40);
			new_sep.Kind = PPCommandItem::kSeparator;
			p_item = &new_sep;
		}
		if(p_item) {
			uint p = 0;
			//@v10.8.5 const PPCommandItem * p_menu = P_Menus_->SearchByID(Data.ID, &p);
			//@v10.8.5 P_Menus_->GetUniqueID(&p_item->ID);
			Data.GetUniqueID(&p_item->ID); // @v10.8.5
			if(parent_id) {
				PPCommandItem * p_fi = Data.SearchByIDRecursive(parent_id, 0);
				PPCommandFolder * p_folder = (p_fi && p_fi->Kind == PPCommandItem::kFolder) ? static_cast<PPCommandFolder *>(p_fi) : 0;
				if(p_folder)
					THROW(p_folder->Add(-1, p_item));
			}
			else {
				THROW(Data.Add(-1, p_item));
			}
			/* @v10.7.6 if(p_menu && p_menu->Kind == PPCommandItem::kFolder)
				THROW(P_Menus_->Update(p, static_cast<PPCommandItem *>(&Data))); */
			{
				THROW(Data.GetCommandList(&cmd_list, 0));
				//cmd_list.lsearch(&p_item->ID, &(p = 0), CMPF_LONG, sizeof(long));
				cmd_list.Search(p_item->ID, &(p = 0));
				ASSIGN_PTR(pPos, p);
			}
			ASSIGN_PTR(pID,  p_item->ID);
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int CommandsDialog::editItem(long pos, long id)
{
	int    ok = -1;
	PPCommandItem * p_item = Data.SearchByIDRecursive(id, 0);
	if(p_item) {
		if(p_item->Kind == PPCommandItem::kCommand)
			ok = EditCmdItem(/*P_Menus_*/&Data, static_cast<PPCommand *>(p_item), IsDesktop); // @v10.8.5 P_Menus_-->&Data
		else if(oneof2(p_item->Kind, PPCommandItem::kFolder, PPCommandItem::kGroup)) // @v10.7.6 PPCommandItem::kGroup
			ok = EditName(p_item->Name);
	}
	/* @v10.7.6 if(ok > 0) {
		uint   p = 0;
		const  PPCommandItem * p_menu = P_Menus_->SearchByID(Data.ID, &p);
		if(p_menu && oneof2(p_menu->Kind, PPCommandItem::kFolder, PPCommandItem::kGroup)) // @v10.7.6 PPCommandItem::kGroup
			THROW(P_Menus_->Update(p, static_cast<PPCommandItem *>(&Data)));
	}
	CATCHZOKPPERR
	*/
	return ok;
}

int CommandsDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(CONFIRM(PPCFM_DELITEM)) {
		long   parent_id = 0;
		const  PPCommandItem * p_item = Data.SearchByIDRecursive_Const(id, &parent_id);
		PPCommandFolder * p_folder = 0;
		if(p_item) {
			uint   p = 0;
			if(parent_id) {
				PPCommandItem * p_pitem = Data.SearchByIDRecursive(parent_id, 0);
				p_folder = (p_pitem && oneof2(p_pitem->Kind, PPCommandItem::kFolder, PPCommandItem::kGroup)) ? static_cast<PPCommandFolder *>(p_pitem) : 0;
			}
			else
				p_folder = &Data;
			if(p_folder && p_folder->SearchByID(id, &p))
				ok = p_folder->Remove(p);
			/* @v10.7.6 if(ok > 0) {
				const PPCommandItem * p_menu = P_Menus_->SearchByID(Data.ID, &(p = 0));
				if(p_menu && oneof2(p_menu->Kind, PPCommandItem::kFolder, PPCommandItem::kGroup))
					THROW(P_Menus_->Update(p, static_cast<PPCommandItem *>(&Data)));
			}*/
		}
	}
	// @v10.7.6 CATCHZOKPPERR
	return ok;
}

PPCommandItem * CommandsDialog::GetItem(long id, long * pPos, PPCommandFolder ** pFolder)
{
	long   parent_id = 0;
	const  PPCommandItem * p_item = Data.SearchByIDRecursive_Const(id, &parent_id);
	PPCommandItem * p_retitem = 0;
	if(p_item) {
		uint   p = 0;
		if(parent_id) {
			PPCommandItem * p_pitem = Data.SearchByIDRecursive(parent_id, 0);
			(*pFolder) = (p_pitem && oneof2(p_pitem->Kind, PPCommandItem::kFolder, PPCommandItem::kGroup)) ? static_cast<PPCommandFolder *>(p_pitem) : 0;
		}
		else
			(*pFolder) = &Data;
		if(*pFolder) {
			p_retitem = ((*pFolder)->SearchByID(id, &p))->Dup();
			ASSIGN_PTR(pPos, p);
		}
	}
	return p_retitem;
}

int CommandsDialog::moveItem(long curPos, long id, int up)
{
	int    ok = -1;
	if(id > 0) {
		long   pos = 0;
		PPCommandFolder * p_folder = 0;
		PPCommandItem * p_item = GetItem(id, &pos, &p_folder);
		if(p_folder) {
			const uint items_count = p_folder->GetCount();
			if(items_count > 1) {
				uint   p;
				uint   nb_pos = 0;
				PPID   mm = 0;
                const  PPCommandItem * p_citem = 0;
				for(p = 0; (p_citem = p_folder->Next(&p));) {
					if(up)
						mm = (p_citem->ID > mm && p_citem->ID < id) ? p_citem->ID : mm;
					else
						mm = ((!mm || p_citem->ID < mm) && p_citem->ID > id) ? p_citem->ID : mm;
				}
				p_citem = p_folder->SearchByID(mm, &nb_pos);
				if(p_citem) {
					PPCommandItem * p_nbitem = p_citem->Dup();
					if(p_nbitem) {
						// @v10.8.5 const  PPCommandItem * p_menu = P_Menus_->SearchByID(Data.ID, &(p = 0));
						p_folder->Remove((uint)pos > nb_pos ? (uint)pos : nb_pos);
						p_folder->Remove((uint)pos > nb_pos ? nb_pos : (uint)pos);
						id           = p_item->ID;
						p_item->ID   = p_nbitem->ID;
						p_nbitem->ID = id;
						p_folder->Add(-1, p_item);
						p_folder->Add(-1, p_nbitem);
						/* @v10.7.6 if(p_menu && oneof2(p_menu->Kind, PPCommandItem::kFolder, PPCommandItem::kGroup))
							P_Menus_->Update(p, static_cast<PPCommandItem *>(&Data));*/
						ZDELETE(p_nbitem);
						ok = 1;
					}
				}
			}
		}
		ZDELETE(p_item);
	}
	return ok;
}
//
// DesktopAssocCommandDialog
//
class DesktopAssocCommandDialog : public TDialog {
	DECL_DIALOG_DATA(PPDesktopAssocCmd);
public:
	DesktopAssocCommandDialog(long pos, PPDesktopAssocCmdPool * pCmdList) : TDialog(DLG_DESKCMDAI), Pos(pos), P_CmdList(pCmdList)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		StrAssocArray cmd_list;
		PPCommandDescr cmd_descr;
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		cmd_descr.GetResourceList(1, cmd_list);
		cmd_list.SortByText();
		setCtrlString(CTL_DESKCMDAI_CODE, Data.Code);
		setCtrlString(CTL_DESKCMDAI_DVCSERIAL, Data.DvcSerial);
		setCtrlString(CTL_DESKCMDAI_PARAM, Data.CmdParam);
		SetupStrAssocCombo(this, CTLSEL_DESKCMDAI_COMMAND, &cmd_list, Data.CmdID, 0, 0);
		AddClusterAssoc(CTL_DESKCMDAI_FLAGS, 0, PPDesktopAssocCmd::fSpecCode);
		AddClusterAssoc(CTL_DESKCMDAI_FLAGS, 1, PPDesktopAssocCmd::fSpecCodePrefx);
		AddClusterAssoc(CTL_DESKCMDAI_FLAGS, 2, PPDesktopAssocCmd::fNonInteractive);
		SetClusterData(CTL_DESKCMDAI_FLAGS, Data.Flags);
		SetupCtrls();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0, pos = 0;
		SString buf;
		GetClusterData(CTL_DESKCMDAI_FLAGS, &Data.Flags);
		sel = CTL_DESKCMDAI_COMMAND;
		getCtrlData(CTLSEL_DESKCMDAI_COMMAND, &Data.CmdID);
		THROW_PP(Data.CmdID, PPERR_INVCOMMAND);
		getCtrlString(sel = CTL_DESKCMDAI_CODE, Data.Code);
		THROW_PP(Data.Code.Len(), PPERR_INVSPECCODE);
		getCtrlString(CTL_DESKCMDAI_DVCSERIAL, Data.DvcSerial);
		getCtrlString(CTL_DESKCMDAI_PARAM, Data.CmdParam);
		ASSIGN_PTR(pData, Data);
		CATCH
			sel = (sel == CTL_DESKCMDAI_CODE && !(Data.Flags & PPDesktopAssocCmd::fSpecCode)) ? CTL_DESKCMDAI_COMMAND : sel;
			ok = (selectCtrl(sel), 0);
		ENDCATCH
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrls();

	const long   Pos;
	const PPDesktopAssocCmdPool * P_CmdList;
};

IMPL_HANDLE_EVENT(DesktopAssocCommandDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmClusterClk) && event.isCtlEvent(CTL_DESKCMDAI_FLAGS)) {
		SetupCtrls();
	}
	else if(event.isCmd(cmWinKeyDown)) {
		long    flags = 0;
		GetClusterData(CTL_DESKCMDAI_FLAGS, &flags);
		if(!(flags & PPDesktopAssocCmd::fSpecCode)) {
			SString buf;
			const KeyDownCommand * p_cmd = static_cast<const KeyDownCommand *>(event.message.infoPtr);
			if(p_cmd && p_cmd->GetKeyName(buf, 1) > 0)
				p_cmd->GetKeyName(buf);
			setCtrlString(CTL_DESKCMDAI_CODE, buf);
		}
	}
	else
		return;
	clearEvent(event);
}

void DesktopAssocCommandDialog::SetupCtrls()
{
	const long flags = GetClusterData(CTL_DESKCMDAI_FLAGS);
	disableCtrl(CTL_DESKCMDAI_CODE, !(flags & PPDesktopAssocCmd::fSpecCode));
	DisableClusterItem(CTL_DESKCMDAI_FLAGS, 1, !(flags & PPDesktopAssocCmd::fSpecCode));
}
//
// DesktopAssocCmdsDialog
//
class DesktopAssocCmdsDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPDesktopAssocCmdPool);
public:
	DesktopAssocCmdsDialog() : PPListDialog(DLG_DESKCMDA, CTL_DESKCMDA_LIST)
	{
		PPCommandDescr cmd_descr;
		cmd_descr.GetResourceList(1, CmdList);
		disableCtrl(CTLSEL_DESKCMDA_DESKTOP, 1);
	}
	DECL_DIALOG_SETDTS()
	{
		StrAssocArray desk_list;
		if(!RVALUEPTR(Data, pData))
			Data.Init(-1);
		PPCommandFolder::GetMenuList(0, &desk_list, 1);
		SetupStrAssocCombo(this, CTLSEL_DESKCMDA_DESKTOP, &desk_list, Data.GetDesktopID(), 0, 0);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		const PPID desktop_id = getCtrlLong(CTLSEL_DESKCMDA_DESKTOP);
		Data.SetDesktopID(desktop_id);
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	int    EditCommandAssoc(long pos, PPDesktopAssocCmd *, PPDesktopAssocCmdPool * pCmdList);

	StrAssocArray CmdList;
};

int DesktopAssocCmdsDialog::setupList()
{
	int    ok = 1;
	PPDesktopAssocCmd assci;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; ok && i < Data.GetCount(); i++) {
		if(Data.GetItem(i, assci)) {
			uint pos = 0;
			if(CmdList.Search(assci.CmdID, &pos) > 0) {
				ss.clear();
				ss.add(assci.Code);
				ss.add(CmdList.Get(pos).Txt);
				if(!addStringToList(i+1, ss.getBuf()))
					ok = 0;
			}
		}
	}
	return ok;
}

int DesktopAssocCmdsDialog::EditCommandAssoc(long pos, PPDesktopAssocCmd * pAsscCmd, PPDesktopAssocCmdPool * pCmdList)
{
	DIALOG_PROC_BODY_P2ERR(DesktopAssocCommandDialog, pos, pCmdList, pAsscCmd)
}

int DesktopAssocCmdsDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	uint   pos = 0;
	PPDesktopAssocCmd cmd_assc;
	if(EditCommandAssoc(-1, &cmd_assc, &Data) > 0) {
		ok = Data.AddItem(&cmd_assc);
		if(ok > 0) {
			pos = Data.GetCount()-1;
			ASSIGN_PTR(pPos, pos);
			ASSIGN_PTR(pID, pos+1);
		}
	}
	return ok;
}

int DesktopAssocCmdsDialog::editItem(long pos, long id)
{
	int    ok = -1;
	PPDesktopAssocCmd cmd_assc;
	if(Data.GetItem(pos, cmd_assc)) {
		ok = EditCommandAssoc(pos, &cmd_assc, &Data);
		if(ok > 0) {
			Data.SetItem(pos, &cmd_assc);
		}
	}
	return ok;
}

int DesktopAssocCmdsDialog::delItem(long pos, long id)
{
	int    r = -1;
	if(pos >= 0 && pos < (long)Data.GetCount())
		r = Data.SetItem(pos, 0);
	return r;
}

// static
int PPDesktop::EditAssocCmdList(long desktopID)
{
	int    ok = -1;
	PPDesktopAssocCmdPool list;
	DesktopAssocCmdsDialog * p_dlg = 0;
	THROW(list.ReadFromProp(desktopID));
	THROW(CheckDialogPtr(&(p_dlg = new DesktopAssocCmdsDialog())));
	p_dlg->setDTS(&list);
	for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
		if(p_dlg->getDTS(&list) > 0) {
			THROW(list.WriteToProp(1));
			valid_data = ok = 1;
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}
//
// MenusDialog
//
int SLAPI EditName(SString & rName)
{
	int    ok = -1;
	SString name = rName;
	SString org_name = name;
	PPInputStringDialogParam isd_param;
	PPLoadText(PPTXT_NEWLABEL, isd_param.InputTitle);
	if(name.C(0) == '@') {
		SString temp_buf;
		if(PPLoadString(name.ShiftLeft(), temp_buf) > 0) {
			name = temp_buf;
			org_name = name;
		}
	}
	if(InputStringDialog(&isd_param, name) > 0 && name.Len()) {
		if(name != org_name) {
			rName = name;
			ok = 1;
		}
	}
	return ok;
}

class EditMenusDlg : public PPListDialog {
	DECL_DIALOG_DATA(PPCommandGroup);
	enum {
		ctlgroupImg   = 1,
		ctlgroupBkgnd = 2
	};
public:
	EditMenusDlg(int isDesktop, long initID) : PPListDialog(DLG_MENULIST, CTL_MENULIST_LIST), IsMaster(PPMaster), IsDesktop(isDesktop)
	{
		SString title;
		uint   title_id = 0;
		if(IsDesktop) {
			CurDict->GetDbSymb(DbSymb);
			showCtrl(CTL_MENULIST_EDMBTN, 0);
			title_id = PPTXT_EDITDESKTOP;
			setSmartListBoxOption(CTL_MENULIST_LIST, lbtSelNotify);
			addGroup(ctlgroupImg, new ImageBrowseCtrlGroup(/*PPTXT_PICFILESEXTS,*/CTL_MENULIST_IMAGE, cmAddImage, cmDelImage, 1));
			addGroup(ctlgroupBkgnd, new ColorCtrlGroup(CTL_MENULIST_BKGND, CTLSEL_MENULIST_BKGND, cmSelBkgnd, CTL_MENULIST_SELBKGND));
			disableCtrls(!IsMaster && ObjRts.CheckDesktopID(0, PPR_INS) == 0, CTL_MENULIST_EDASSCBTN, CTL_MENULIST_EDASSCCBTN, 0L);
		}
		else {
			RECT   rect, img_rect;
			::GetWindowRect(H(), &rect);
			::GetWindowRect(::GetDlgItem(H(), CTL_MENULIST_IMAGE), &img_rect);
			rect.bottom -= (img_rect.bottom - img_rect.top);
			::MoveWindow(H(), rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 1);
			showCtrl(CTL_MENULIST_EDASSCBTN,  0);
			showCtrl(CTL_MENULIST_EDASSCCBTN, 0);
			showCtrl(CTL_MENULIST_IMAGE,      0);
			showCtrl(CTL_MENULIST_ADDIMG,     0);
			showCtrl(CTL_MENULIST_DELIMG,     0);
			showCtrl(CTL_MENULIST_BKGND,      0);
			showCtrl(CTLSEL_MENULIST_BKGND,   0);
			showCtrl(CTL_MENULIST_SELBKGND,   0);
			title_id = PPTXT_EDITMENU;
			setupPosition();
		}
		PPLoadText(title_id, title);
		setTitle(title);
		PrevID = 0;
		InitID = initID;
		updateList(-1);
	}
	DECL_DIALOG_SETDTS()
	{
		PPID   cur_id = 0;
		RVALUEPTR(Data, pData);
		AddClusterAssoc(CTL_MENULIST_GRPFLAGS, 0, PPCommandItem::fNotUseDefDesktop);
		SetClusterData(CTL_MENULIST_GRPFLAGS, static_cast<long>(Data.Flags));
		AddClusterAssoc(CTL_MENULIST_FLAGS, 0, PPCommandItem::fBkgndGradient);
		if(InitID)
			updateList(InitID, 0);
		else
			updateList(-1);
		getSelection(&cur_id);
		LoadCfg(InitID ? InitID : cur_id);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		LoadCfg(0);
		Data.Flags = static_cast<int16>(GetClusterData(CTL_MENULIST_GRPFLAGS));
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		long   id = 0;
		if(event.isCmd(cmEditMenu)) {
			if(P_Box && P_Box->getCurID(&id) > 0)
				if(!EditMenu(id))
					PPError();
		}
		else if(event.isCmd(cmEditAsscCmdCommonList) || event.isCmd(cmEditAsscCmdList)) {
			const int edit_by_desk = BIN(TVCMD == cmEditAsscCmdList);
			if(edit_by_desk)
				getSelection(&id);
			if(!edit_by_desk || edit_by_desk && id)
				PPDesktop::EditAssocCmdList(id);
		}
		else if(event.isCmd(cmLBItemSelected)) {
			getSelection(&id);
			LoadCfg(id);
		}
		else
			return;
		clearEvent(event);
	}
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	int    EditMenu(long id);
	int    IsMenuUsed(PPID obj, PPID menuID, int isDesktop);
	int    LoadCfg(long id);

	const  int IsMaster;
	int    IsDesktop;
	long   InitID;
	long   PrevID;
	SString DbSymb;
};

int EditMenusDlg::LoadCfg(long id)
{
	if(IsDesktop && PrevID != id) {
		uint   pos = 0;
		long   flags = 0;
		ImageBrowseCtrlGroup::Rec rec;
		ColorCtrlGroup::Rec color_rec;
		PPCommandGroup * p_desk = 0;
		if(PrevID) {
			p_desk = Data.GetDesktop(PrevID);
			if(p_desk) {
				GetClusterData(CTL_MENULIST_FLAGS, &flags);
				p_desk->Flags = static_cast<uint16>(flags);
				getGroupData(ctlgroupBkgnd, &color_rec);
				p_desk->Icon.Z().Cat(color_rec.C);
				getGroupData(ctlgroupImg, &rec);
				p_desk->SetLogo(rec.Path);
			}
		}
		rec.Path.Z();
		color_rec.C = 0;
		flags    = 0;
		PrevID   = id;
		p_desk   = Data.GetDesktop(id);
		if(p_desk) {
			rec.Path = p_desk->GetLogo();
			flags    = static_cast<long>(p_desk->Flags);
			color_rec.C = p_desk->Icon.ToLong();
		}
		setGroupData(ctlgroupImg, &rec);
		SetClusterData(CTL_MENULIST_FLAGS, flags);
		{
			color_rec.SetupStdColorList();
 			color_rec.C = (color_rec.C == 0) ? static_cast<long>(PPDesktop::GetDefaultBgColor()) : color_rec.C;
			setGroupData(ctlgroupBkgnd, &color_rec);
		}
	}
	{
		const PPRights & r_rt = ObjRts;
		disableCtrls(!IsMaster && r_rt.CheckDesktopID(0, PPR_INS) == 0,
			CTL_MENULIST_FLAGS, CTL_MENULIST_IMAGE, CTLSEL_MENULIST_BKGND, CTL_MENULIST_SELBKGND, 0L);
		showCtrl(CTL_MENULIST_ADDIMG, IsMaster || r_rt.CheckDesktopID(0, PPR_INS));
		showCtrl(CTL_MENULIST_DELIMG, IsMaster || r_rt.CheckDesktopID(0, PPR_INS));
	}
	return 1;
}

int EditMenusDlg::setupList()
{
	int    ok = 1;
	StrAssocArray items;
	THROW(PPCommandFolder::GetMenuList(&Data, &items, IsDesktop));
	for(uint i = 0; i < items.getCount(); i++) {
		StrAssocArray::Item item = items.Get(i);
		StringSet ss(SLBColumnDelim);
		ss += item.Txt;
		THROW_SL(P_Box->addItem(item.Id, ss.getBuf()))
	}
	CATCHZOK
	return ok;
}

int EditMenusDlg::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	if(IsMaster || ObjRts.CheckDesktopID(0, PPR_INS)) {
		long   parent_id = 0;
		SString name;
		while(ok < 0 && SelectMenu(&parent_id, &name, IsDesktop ? SELTYPE_DESKTOPTEMPL : SELTYPE_MENUTEMPL, &Data) > 0) {
			// @v10.8.1 Блок скорректирован - были дефекты в обработке рабочих столов
			// @v10.8.1 long   id = 0;
			const  PPCommandItem * p_item = Data.SearchByID(parent_id, 0);
			PPCommandGroup new_desk;
			PPCommandGroup new_menu2;
			PPCommandGroup * p_new_group = 0;
			if(IsDesktop) {
				if(p_item && p_item->Kind == PPCommandItem::kGroup) {
					new_desk = *static_cast<const PPCommandGroup *>(p_item);
					new_desk.SetLogo(0);
				}
				new_desk.Type = PPCommandGroup::tDesk;
				new_desk.SetDbSymb(DbSymb);
				new_desk.GetUniqueID(&new_desk.ID); // @v10.8.1 
				p_new_group = &new_desk;
			}
			else {
				if(p_item && p_item->Kind == PPCommandItem::kFolder) {
					//new_menu = *static_cast<const PPCommandFolder *>(p_item);
					new_menu2.PPCommandFolder::Copy(*static_cast<const PPCommandFolder *>(p_item));
					new_menu2.Kind = PPCommandItem::kGroup; // PPCommandFolder::Copy has changed Kind so we have to revert it
					new_menu2.Flags = p_item->Flags;
					new_menu2.Icon = p_item->Icon;
				}
				else if(parent_id > DEFAULT_MENUS_OFFS) {
					PPCommandFolder new_menu_folder;
					MenuResToMenu(parent_id - DEFAULT_MENUS_OFFS, &new_menu_folder);
					new_menu2.PPCommandFolder::Copy(new_menu_folder);
					new_menu2.Kind = PPCommandItem::kGroup; // PPCommandFolder::Copy has changed Kind so we have to revert it
					new_menu2.Flags = new_menu_folder.Flags;
					new_menu2.Icon = new_menu_folder.Icon;
				}
				new_menu2.Type = PPCommandGroup::tMenu;
				Data.GetUniqueID(&new_menu2.ID);
				new_menu2.DbSymb = "undefined";
				p_new_group = &new_menu2;
			}
			assert(p_new_group);
			if(p_new_group) {
				p_new_group->Name = name;
				p_new_group->GenerateGuid();
				if(Data.Add(-1, p_new_group))
					ok = 1;
				else
					PPErrorZ();
			}
		}
	}
	else
		ok = PPErrorZ();
	return ok;
}

int EditMenusDlg::editItem(long pos, long id)
{
	int    ok = -1;
	if(IsMaster || ObjRts.CheckDesktopID(id, PPR_MOD)) {
		uint ipos = 0;
		const PPCommandItem * p_item = Data.SearchByID(id, &ipos);
		PPCommandItem * p_savitem = (p_item && p_item->Kind == PPCommandItem::kGroup) ? p_item->Dup() : 0;
		if(p_savitem) {
			while(ok < 0 && EditName(p_savitem->Name) > 0) {
				if(Data.Update(ipos, p_savitem))
					ok = 1;
				else
					PPError();
			}
		}
		ZDELETE(p_savitem);
	}
	else
		ok = PPErrorZ();
	return ok;
}

int EditMenusDlg::delItem(long pos, long id)
{
	int    ok = -1;
	uint   ipos = 0;
	SString str_guid;
	if(IsMaster || ObjRts.CheckDesktopID(id, PPR_MOD)) {
		const PPCommandItem * p_item = Data.SearchByID(id, &ipos);
		if(p_item) {
			const int is_confirmed = IsDesktop ? CONFIRM(PPCFM_DELDESKTOP) : CONFIRM(PPCFM_DELMENU);
			if(is_confirmed) {
				if(IsMenuUsed(PPOBJ_USR, id, IsDesktop) || IsMenuUsed(PPOBJ_USRGRP, id, IsDesktop)) {
					ok = 0;
					PPErrCode = IsDesktop ? PPERR_DESKTOPBLOCKED : PPERR_MENUBLOCKED;
				}
				else {
					//@erik v10.7.3 {
					// 
					const PPCommandGroup * p_cgroup = Data.GetDesktop(id);
					if(p_cgroup && p_cgroup->GetGuid().ToStr(S_GUID::fmtIDL, str_guid)) {
						PPCommandMngr * p_mgr = GetCommandMngr(1, 1, 0);
						if(p_mgr->DeleteDesktopByGUID(str_guid, PPCommandMngr::fRWByXml))
							ok = Data.Remove(ipos);
					}
					// } @erik 
				}
			}
		}
	}
	else
		ok = 0;
	if(!ok)
		PPError();
	return ok;
}

int EditMenusDlg::IsMenuUsed(PPID obj, PPID menuID, int isDesktop)
{
	Reference * p_ref = PPRef;
	int    used = isDesktop ? BIN(LConfig.DesktopID == menuID) : 0;
	for(PPID id = 0; !used && p_ref->EnumItems(obj, &id) > 0;) {
		PPConfig cfg;
		if(p_ref->GetProperty(obj, id, PPPRP_CFG, &cfg, sizeof(cfg)) > 0)
			used = isDesktop ? BIN(cfg.DesktopID == menuID) : BIN(cfg.MenuID == menuID);
	}
	return used;
}

int EditMenusDlg::EditMenu(long id)
{
	int    ok = -1;
	uint   pos = 0;
	CommandsDialog * p_dlg = 0;
	PPCommandItem * p_item = const_cast<PPCommandItem *>(Data.SearchByID(id, &pos)); // @badcast
	PPCommandGroup * p_edited_group = 0;
	PPCommandGroup * p_menus = 0;
	if(p_item) {
		/*if(p_item->Kind == PPCommandItem::kFolder) 
			p_folder = static_cast<PPCommandFolder *>(p_item->Dup());
		else*/
		if(p_item->Kind == PPCommandItem::kGroup)
			p_edited_group = static_cast<PPCommandGroup *>(p_item); // @v10.7.6 static_cast<PPCommandGroup *>(p_item->Dup())-->p_item
	}
	if(p_edited_group) {
		p_menus = static_cast<PPCommandGroup *>(Data.Dup());
		THROW(CheckDialogPtr(&(p_dlg = new CommandsDialog(/*p_menus,*/IsDesktop))));
		p_dlg->setDTS(p_edited_group);
		if(ExecView(p_dlg) == cmOK) {
			if(p_dlg->getDTS(p_edited_group)) {
				//Data = *p_menus;
				ok = 1;
			}
		}
	}
	CATCHZOK
	// @v10.7.6 ZDELETE(p_edited_group);
	ZDELETE(p_menus);
	delete p_dlg;
	return ok;
}

int SLAPI EditMenus(PPCommandGroup * pData, long initID, int isDesktop)
{
	int    ok = -1;
	PPCommandGroup command_group;
	EditMenusDlg * p_dlg = new EditMenusDlg(isDesktop, initID);
	PPCommandMngr * p_mgr = 0;
	THROW_MEM(p_dlg);
	THROW(CheckDialogPtr(&p_dlg));
	if(pData)
		command_group = *pData;
	else {
		THROW(p_mgr = GetCommandMngr(0, isDesktop, 0));
		//if(isDesktop) {
			THROW(p_mgr->Load__2(&command_group, PPCommandMngr::fRWByXml)); //@erik v10.7.2
		//}
		//else {
		//	THROW(p_mgr->Load__(&command_group));
		//}
	}
	p_dlg->setDTS(&command_group);
	if(ExecView(p_dlg) == cmOK) {
		p_dlg->getDTS(&command_group);
		if(pData)
			*pData = command_group;
		else {
			//if(isDesktop) {
				THROW(p_mgr->Save__2(&command_group, PPCommandMngr::fRWByXml)); //@erik v10.7.2 Save__ => Save__2
			//}
			//else {
			//	THROW(p_mgr->Save__(&command_group));
			//}
		}
		ok = 1;
	}
	CATCHZOKPPERR
	delete p_dlg;
	ZDELETE(p_mgr);
	return ok;
}

int SLAPI EditMenusFromFile()
{
	int    ok = -1;
	PPCommandMngr * p_mgr = 0;
	SString path;
	PPCommandGroup menus;
	if(PPOpenFile(PPTXT_FILPAT_MENU, path, 0, 0) > 0) {
		{
			THROW(p_mgr = GetCommandMngr(1, 0, path));
			//THROW(p_mgr->Load__(&menus)); //@erik v10.7.5
			THROW(p_mgr->Load__2(&menus, PPCommandMngr::fRWByXml));//@erik v10.7.5
			ZDELETE(p_mgr);
		}
		if((ok = EditMenus(&menus, 0, 0)) > 0) {
			THROW(p_mgr = GetCommandMngr(0, 0, path));
			/*THROW(p_mgr->Save__(&menus));*/ //@erik v10.7.5
			THROW(p_mgr->Save__2(&menus, PPCommandMngr::fRWByXml)); //@erik v10.7.5
			ZDELETE(p_mgr);
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete p_mgr;
	return ok;
}
//
//
//
int SelectMenu(long * pID, SString * pName, int selType, const PPCommandGroup * pGrp)
{
	int    ok = -1;
	const  int is_desktop = BIN(oneof2(selType, SELTYPE_DESKTOP, SELTYPE_DESKTOPTEMPL));
	SString db_symb, buf, left, right;
	StrAssocArray ary;
	THROW(PPCommandFolder::GetMenuList(pGrp, &ary, is_desktop));
	if(selType == SELTYPE_MENUTEMPL) {
		TVRez * p_rez = P_SlRez;
		uint   locm_id = 0;
		PPLoadText(PPTXT_DEFAULTMENUS, buf);
		StringSet ss(';', buf);
		for(uint i = 0; ss.get(&i, buf) > 0;) {
			buf.Divide(',', left, right);
			ary.Add(left.ToLong() + DEFAULT_MENUS_OFFS, right);
		}
		fseek(p_rez->getStream(), 0, SEEK_SET);
		for(ulong pos = 0; p_rez->enumResources(0x04, &locm_id, &pos) > 0;) {
			long _id = static_cast<long>(locm_id) + DEFAULT_MENUS_OFFS;
			if(ary.GetText(_id, buf) <= 0)
				ary.Add(_id, buf.Z().Cat(_id - DEFAULT_MENUS_OFFS));
		}
	}
	{
		left.Z();
		right.Z();
		if(PPGetSubStr(PPTXT_CMDPOOLSEL, selType, buf))
			buf.Divide(',', left, right);
		THROW(ok = AdvComboBoxSelDialog(&ary, left, right, pID, pName, 0));
	}
	CATCHZOK
	return ok;
}
//
// Load menus from resource
//
struct MITH {
	uint   versionNumber;
	uint   offset;
};

struct MIT {
	uint   mtOption;
	uint   mtID;
	char   mtString[256];
};

static int SLAPI readMIT(TVRez & rez, MIT & mit, long ofs)
{
	if(rez.getStreamPos() >= ofs)
		return 0;
	else {
		mit.mtOption = rez.getUINT();
		if(!(mit.mtOption & MF_POPUP))
			mit.mtID = rez.getUINT();
		rez.getString(mit.mtString, 1);
		return 1;
	}
}

void readMenuRez(HMENU hm, TVRez * rez, long length)
{
	MIT  mit;
	SString menu_text;
	SString temp_buf;
	while(readMIT(*rez, mit, length)) {
		if(!mit.mtOption && !mit.mtID && !mit.mtString[0])
			AppendMenu(hm, MF_SEPARATOR, 0, 0);
		else {
			menu_text = mit.mtString;
			if(menu_text.Len() > 2 && menu_text[0] == '@') {
				if(PPLoadString(menu_text+1, temp_buf))
					menu_text = temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			}
			if(mit.mtOption & MF_POPUP) {
				HMENU hPopMenu = CreateMenu();
				AppendMenu(hm, (mit.mtOption | MF_STRING) & ~MF_END, reinterpret_cast<UINT_PTR>(hPopMenu), SUcSwitch(menu_text));
				readMenuRez(hPopMenu, rez, length);
			}
			else {
				AppendMenu(hm, (mit.mtOption | MF_STRING) & ~MF_END, mit.mtID, SUcSwitch(menu_text));
				if(mit.mtOption & MF_END)
					return;
			}
		}
	}
}

//void SLAPI ReadMenu(HMENU hm, PPID parentID, PPCommandFolder * pMenu, StrAssocArray * pItems) //@erik v10.7.5
void SLAPI ReadMenu(HMENU hm, PPID parentID, PPCommandGroup * pMenu, StrAssocArray * pItems) //@erik v10.7.5
{
	SString name;
	if(pMenu && pItems) {
		SString temp_buf;
		for(uint i = 0; i < pItems->getCount(); i++) {
			StrAssocArray::Item item_ = pItems->Get(i);
			if(item_.ParentId == parentID) {
				const PPCommandItem * p_item = pMenu->SearchByIDRecursive_Const(item_.Id, 0);
				if(p_item) {
					char   name_buf[256];
					name = p_item->Name;
					if(name.C(0) == '@' && PPLoadString(name.ShiftLeft(), temp_buf) > 0)
						name = temp_buf;
					name.Transf(CTRANSF_INNER_TO_OUTER);
					name.CopyTo(name_buf, sizeof(name_buf));
					if(p_item->Kind == PPCommandItem::kFolder) {
						HMENU h_pop_menu = CreateMenu();
						::AppendMenu(hm, MF_POPUP|MF_STRING, reinterpret_cast<UINT_PTR>(h_pop_menu), SUcSwitch(name_buf)); // @unicodeproblem
						ReadMenu(h_pop_menu, p_item->ID, pMenu, pItems); // @recursion
					}
					else if(p_item->Kind == PPCommandItem::kSeparator)
						::AppendMenu(hm, MF_SEPARATOR, 0, 0);
					else {
						PPCommandDescr descr;
						descr.LoadResource(static_cast<const PPCommand *>(p_item)->CmdID);
						::AppendMenu(hm, MF_STRING, static_cast<UINT_PTR>(descr.MenuCm), SUcSwitch(name_buf)); // @unicodeproblem
					}
				}
			}
		}
	}
}

HMENU SLAPI PPLoadMenu(TVRez * rez, long menuID, int fromRc, int * pNotFound)
{
	int    not_found = 1;
	HMENU  m = 0;
	StrAssocArray * p_items = 0;
	PPCommandGroup * p_menu = 0;
	if(!fromRc && menuID) {
		PPCommandMngr * p_mgr = GetCommandMngr(1, 0);
		PPCommandGroup menus;
		//if(p_mgr && p_mgr->Load__(&menus)>0) { //@erik v10.7.5
		if(p_mgr && p_mgr->Load__2(&menus, PPCommandMngr::fRWByXml)>0) {//@erik v10.7.5
			const PPCommandItem * p_item = menus.SearchByID(menuID, 0);
			m = CreateMenu();
			//p_menu = (p_item && p_item->Kind == PPCommandItem::kFolder) ? static_cast<PPCommandFolder *>(p_item->Dup()) : 0; //@erik v10.7.5
			p_menu = (p_item && p_item->Kind == PPCommandItem::kGroup) ? static_cast<PPCommandGroup*>(p_item->Dup()) : 0;
			if(p_menu && p_menu->Type == PPCommandGroup::tMenu && (p_items = new StrAssocArray) != 0 && p_menu->GetCommandList(p_items, 0)) { // add p_menu->Type == PPCommandGroup::tMenu // @erik v10.7.6
				ReadMenu(m, 0, p_menu, p_items);
				not_found = 0;
			}
		}
		ZDELETE(p_mgr);
	}
	else if(rez != 0) {
		m = CreateMenu();
		MITH   mith;
		long   length, menuOfs;
		if(rez->findResource(menuID, 0x04, &menuOfs, &length)) {
			length += menuOfs;
			mith.versionNumber = rez->getUINT();
			mith.offset = rez->getUINT();
			fseek(rez->getStream(), mith.offset, SEEK_CUR);
			readMenuRez(m, rez, length);
			not_found = 0;
		}
	}
	if(m) {
		HMENU  h_popup = CreateMenu();
		SString temp_buf;
		PPLoadStringS("cmd_window", temp_buf).Transf(CTRANSF_INNER_TO_OUTER);
		AppendMenu(m, MF_POPUP | MF_STRING, (UINT)h_popup, SUcSwitch(temp_buf));
		UserInterfaceSettings uiset;
		uiset.Restore();
		PPLoadStringS("cmd_menutree", temp_buf).Transf(CTRANSF_INNER_TO_OUTER);
		::AppendMenu(h_popup, ((uiset.Flags & uiset.fShowLeftTree) ? MF_UNCHECKED : MF_CHECKED)|MF_STRING, cmShowTree, SUcSwitch(temp_buf));
		PPLoadStringS("cmd_toolpane", temp_buf).Transf(CTRANSF_INNER_TO_OUTER);
		::AppendMenu(h_popup, MF_CHECKED | MF_STRING, cmShowToolbar, SUcSwitch(temp_buf));
	}
	ASSIGN_PTR(pNotFound, not_found);
	ZDELETE(p_items);
	ZDELETE(p_menu);
	return m;
}

void MenuResToMenu(PPCommandFolder * pFold, LAssocArray * pCmdDescrs, TVRez * rez, long length)
{
	MIT    mit;
	while(readMIT(*rez, mit, length)) {
		if(mit.mtOption & MF_POPUP) {
			PPCommandFolder fold;
			(fold.Name = mit.mtString).Transf(CTRANSF_OUTER_TO_INNER);
			MenuResToMenu(&fold, pCmdDescrs, rez, length);
			pFold->Add(-1, static_cast<const PPCommandItem *>(&fold));
		}
		else if(!mit.mtOption && !mit.mtID && !mit.mtString[0])
			pFold->AddSeparator(-1);
		else {
			uint pos = 0;
			long menu_cm = mit.mtID;
			PPCommand cmd;
			(cmd.Name = mit.mtString).Transf(CTRANSF_OUTER_TO_INNER);
			if(pCmdDescrs->lsearch(&menu_cm, &pos, CMPF_LONG, sizeof(long)) > 0)
				cmd.CmdID = pCmdDescrs->at(pos).Key;
			pFold->Add(-1, static_cast<const PPCommandItem *>(&cmd));
			if(mit.mtOption & MF_END)
				return;
		}
	}
}

int SLAPI MenuResToMenu(uint resMenuID, PPCommandFolder * pMenu)
{
	int    ok = -1;
	TVRez * p_slrez = P_SlRez;
	MITH   mith;
	long   length = 0, menuOfs = 0;
	LAssocArray descrs;
	THROW(PPCommandDescr::GetResourceList(descrs));
	if(p_slrez->findResource(resMenuID, 0x04, &menuOfs, &length)) {
		length += menuOfs;
		mith.versionNumber = p_slrez->getUINT();
		mith.offset = p_slrez->getUINT();
		fseek(p_slrez->getStream(), mith.offset, SEEK_CUR);
		MenuResToMenu(pMenu, &descrs, p_slrez, length);
		ok = 1;
	}
	CATCHZOK
	return ok;
}
