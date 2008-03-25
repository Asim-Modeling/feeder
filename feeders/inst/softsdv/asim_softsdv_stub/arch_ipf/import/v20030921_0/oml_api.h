/*******************************************************************************
 *File: OML_API.h
 *
 * 
 ******************************************************************************/

/*
 * Copyright (C) 2004-2006 Intel Corporation
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef _OMLAPI_H
#define _OMLAPI_H

#include "vpc_types.h"
#include "EM_tools.h"

#ifdef __cplusplus
extern "C" {
#endif
/* this enum is used only to define the maximum number of event and data timers in GE 
   it is in enum since this is the way of the odb_build perl script to identify inherit enums 
*/
typedef enum timer_num_e
{
	OML_TIMER_NUM = 101
} OML_timer_num_t;

typedef enum tlb_entry_num_e
{
      OML_MAX_TLB_ENTRIES_NUM = 255
} OML_tlb_entry_num_t;


#define OML_NULL_LEVEL 0
#define OML_CLASS_LEVEL 0

#define OML_MAX_TREE_LEVEL 50 
#define OML_NULL_ODB 0xffffffff	 /* index of 'NULL ODB' */
#define OML_BASE_ODB 0			 /* handle to default ODB */
#define OML_NULL_HANDLE 0		 /* same value for compatability reasons */

/* general system tokens: */
#define GE_CLASS_DATA	    3001
#define GE_CLASS_EVENT      3002
#define GE_CLASS_NOTIFIER	3003
#define	OML_END_TOKEN 0xffffffff  
#define GE_ROOT				3004


typedef const char OML_name_t;


typedef unsigned OML_handle_t;


/* this enum is used as a parameter for owner_state_update_func	to identify
   the change in the object state */
typedef enum
{
	OML_OBJ_ACTION_REQUIRED,
	OML_OBJ_ACTION_ACTIVATED,
	OML_OBJ_ACTION_DEACTIVATED
} OML_state_transition_t;

typedef enum
{
	OML_NEXT_DEFAULT,	
	OML_NEXT_SON,
	OML_NEXT_BROTHER,	
	OML_NEXT_ITEM
}OML_next_mode_t;

/* Filters for get_next function */

#define OML_NEXT_NO_FILTER	OML_NO_STATE	
#define	OML_NEXT_DEFINED	(1 << (OML_STATE_DEFINED-1))	
#define	OML_NEXT_OPENED		(1 << (OML_STATE_OPENED-1)) 		
#define	OML_NEXT_CLOSED		(1 << (OML_STATE_CLOSED-1)) 
#define	OML_NEXT_AVAILABLE	(1 << (OML_STATE_AVAILABLE-1)) 	
#define	OML_NEXT_ACTIVE		(1 << (OML_STATE_ACTIVE-1)) 
#define OML_JUST_ODB        (1 << (OML_LAST_STATE-1))


typedef enum
{
	OML_NO_STATE,
	OML_STATE_DEFINED,			
	OML_STATE_OPENED,			
	OML_STATE_CLOSED,			
	OML_STATE_AVAILABLE,			
	OML_STATE_ACTIVE,
	OML_LAST_STATE
} OML_state_t;

/* ODB structures*/
typedef enum
{	
	OML_ID_CLASS_LEVEL = OML_CLASS_LEVEL,
	OML_ID_ITEM_LEVEL  = OML_MAX_TREE_LEVEL+1,
	OML_ID_FAMILY_LEVEL,							   /* used only as a return value for get_property */
	OML_ID_LAST_TOKEN_LEVEL  = OML_MAX_TREE_LEVEL*10  /*should be a big-enough number 
												to defer from normal range of 
												token id level (1..OML_MAX_TREE_LEVEL)*/
}OML_token_type_t;

/* A type definition of the objects properties. */
typedef enum{
	OML_OBJ_NAME,
	OML_OBJ_ALIAS_NAME,
	OML_OBJ_STATE ,
	OML_VALUE_TYPE,
	OML_VALUE_SIZE,
	OML_OBJ_TOKEN_TYPE,
	OML_OBJ_VALUE_ADDR,
	OML_OBJ_OWNER_INFO,
	OML_OBJ_SET_ALLOWED,
	OML_OBJ_BUSY,
	OML_VALUE_IS_LOCAL,
	OML_CLASS_TYPE,
	OML_OBJ_OWNER
} OML_obj_prop_t;

/***************************************
 ****** callback functions typedefs ****
 ***************************************/

/* This type is used for the owner callback function for notification of 
   changes in the object state.
    Parameters
   oml_id       The OML tool ID, for debug purpose only.
   obj_hndl     The object who's state changed. Can be a family or an item.
   transition_action    The action causing the state change. This action 
						determines the new object state.
   *owner_info  Information attached to the object by the owner.
*/
typedef GE_stat_t (*OML_owner_state_update_func_t) 
								(GE_tool_id_t oml_id,  
								 OML_handle_t         obj_hndl,
								 OML_state_transition_t transition_action,
								 void          *owner_info);

                      



/* This type is used for the client callback function for receiving 
   notification on event occurrence or notifier value changes. 
   Parameters:
   oml_id       The OML tool ID, for debug purpose only.
   owner_id     The tool that sent the event or changed the notifier value. 
   obj_hndl     The object responsible. Must specify an event or a notifier 
                item handle.
*/	  
typedef GE_stat_t (*OML_client_receive_func_ptr_t) 
								(GE_tool_id_t     tool_id, 
								 GE_tool_id_t     owner_id,
								 OML_handle_t     obj_hndl) ;

/* This type is used as the previous with the diffrence that now the functions
   accepts client_info parrameter. 
   Parameters:
   oml_id       The OML tool ID, for debug purpose only.
   owner_id     The tool that sent the event or changed the notifier value. 
   obj_hndl     The object responsible. Must specify an event or a notifier 
                item handle.
   client_info  The pointer to the client parameter.
*/	  
typedef GE_stat_t (*OML_client_receive_func_ptr_with_client_info_t) 
						        (unsigned     tool_id, 
						        GE_tool_id_t     owner_id,
						        OML_handle_t     obj_hndl,
                                void *        client_info) ;


/* This type is used for the owner callback functions for getting the data 
   value, in case it is held by the owner and not by OML. 
   Parameters:
   oml_id       The OML tool ID, for debug purpose only. 
   data_hndl    The data object for getting.
   *owner_info  Information attached to the object by the owner. 
   buf_size     The size of the buffer.
   *buff_ptr    The buffer that should contain the value. 
*/
typedef unsigned (*OML_val_get_clbk_t) (GE_tool_id_t  tool_id,  
										OML_handle_t  data_hndl,
										void          *owner_info,
										unsigned      buff_size_ptr,
										void          *buff_ptr);


/* This type is used for owner callback functions for setting the data value, 
   in case it is held by the owner and not by OML. 
   Parameters
   oml_id       The OML tool ID, for debug purpose only. 
   data_hndl    The data object for setting.
   *owner_info  Information attached to the object by the owner.
   *buff_ptr    The buffer that should contain the value. Since the size of 
                data types except strings is known to OML, the size will         not be given.
*/

typedef unsigned (*OML_val_set_clbk_t) (GE_tool_id_t   oml_id,  
										OML_handle_t   data_hndl,
										void           *owner_info,
										void const     *buff_ptr);



typedef void (*OML_fatal_error_t) (GE_tool_id_t    tool_id,
								   GE_stat_t       err_code,
								   char*           message_str) ;

typedef int  (*OML_usr_message_t) (GE_tool_id_t     tool_id,
								GE_mssg_t        message_type,
								char*            message_str);


typedef char* (*OML_tool_name_t) (GE_tool_id_t tool_id);



/********************************************************************/
/*                        Function prototypes                       */
/********************************************************************/


#ifdef OML_DLL
#  define OML_EXPIMP DLLEXPORT
#else
#  define OML_EXPIMP DLLIMPORT
#endif //OML_DLL

#ifdef OML_DLL
#  ifdef OML_API
#    define OML_EXTERN OML_EXPIMP
#  else
#    define OML_EXTERN 
#  endif //OML_API
#else 
#  ifdef WIN32
#    define OML_EXTERN OML_EXPIMP
#  else 
#    define OML_EXTERN extern  // Because DLLIMPORT is empty for Linux host
#  endif //WIN32
#endif //OML_DLL


#define OML_MAX_NAME	500

/***************************************
 ****** external global variables   ****
 ***************************************/

#if NO_USER_ODB
OML_EXTERN unsigned oml_last_hndl; /* does need to be exported??? */
OML_EXTERN unsigned oml_last_token;
OML_EXTERN unsigned oml_test_flag;
OML_EXTERN unsigned oml_event_num;
OML_EXTERN unsigned oml_event_start;
OML_EXTERN unsigned oml_data_num;
OML_EXTERN unsigned oml_data_start;
OML_EXTERN unsigned oml_notifier_num;
OML_EXTERN unsigned oml_notifier_start;
#endif 






/*********************************************
 ****** API functions                     ****
 *********************************************/
 


OML_EXPIMP void oml_init(GE_tool_id_t     tool_id);

OML_EXPIMP GE_stat_t oml_load_ODB(  GE_tool_id_t     tool_id,
								    char *ODB_load_name);

OML_EXPIMP void oml_config(GE_tool_id_t  tool_id);

OML_EXPIMP void oml_config_check(GE_tool_id_t  tool_id);

OML_EXPIMP void oml_terminate(GE_tool_id_t     tool_id);

OML_EXPIMP GE_stat_t oml_get_version(EM_library_version_t *oml_version);

OML_EXPIMP GE_stat_t oml_get_new_tool_id(int* tool_id);

OML_EXPIMP OML_handle_t oml_obj_translate_name(GE_tool_id_t  tool_id,
										   	   OML_name_t*   name_str);
OML_EXPIMP GE_stat_t oml_obj_get_name(OML_handle_t handle,
							char*   name_buf, unsigned* buf_size);

OML_EXPIMP OML_handle_t   oml_obj_get_hndl(GE_tool_id_t   tool_id,
								 OML_handle_t   family_hndl, ...);

OML_EXPIMP unsigned	oml_obj_get_token_id(GE_tool_id_t  tool_id,
								OML_handle_t  obj_hndl,
								unsigned      token_level);

OML_EXPIMP GE_stat_t oml_is_valid_hndl(GE_tool_id_t  tool_id,
									   OML_handle_t  obj_hndl);

OML_EXPIMP void oml_obj_get_tokens(GE_tool_id_t  tool_id,
					   OML_handle_t  obj_hndl,
					   unsigned      tokens_arr[]);

OML_EXPIMP OML_handle_t oml_obj_get_next_hndl(GE_tool_id_t	tool_id,
										 OML_next_mode_t	mode,
										 unsigned			filter,
										 OML_handle_t		start_hndl, 
										 OML_handle_t		current_obj_hndl);


OML_EXPIMP OML_handle_t oml_obj_get_ODB(GE_tool_id_t	tool_id,
									OML_handle_t  obj_hndl);

OML_EXPIMP OML_handle_t oml_find_first(OML_name_t*  name);

OML_EXPIMP OML_handle_t oml_find_next(OML_name_t*  name, OML_handle_t last_hndl);



/***********************/
/*** owner functions ***/
/***********************/
/*-------------------------------------------------------------------------------
* Name: oml_obj_set_owner
* =======================
* Description
* This function is used by the object owner to initialize the object, or family
* ofobjects, according to the obj_hndl. Initializing an object is allowed only
* once during the TI stage. (oml_reset is the only function that clears object's owner.)
* The object's state will be "opened" after the initialization was completed
* successfully. (Notifiers are always at "active" state).
* During the initialization, the owner will pass the pointer to the callback
* function: state_update_clbk as a parameter. This function is used by OML to
* update the owner on object's states change.
*  
* Parameters:
* tool_id	- Used to identify the object owner.
* obj_hndl	- Used to identify the object to be initialized. The obj_hndl can
*			  indicates both family or items.
* *state_update_clbk - Pointer to a function that will be called to update the
*					   owner on object's state changes. Will be ignored in case
*					   that the object is a notifier.
*
*
* Return Values:
* GE_OK						Function worked correctly.
* OML_ERR_OBJ_RE_OPEN		Object was already initialized by the same tool. 
*							Only if all items under family.
* 
* Fatal Conditions
* OML_ERR_INVALID_GE_STAGE	Function was called in wrong OML stage.
* OML_ERR_INVALID_HNDL		Handle is not known to OML.
* OML_ERR_NULL_FUNC			Function pointer is null, not relevant for notifiers.
* OML_ERR_NOT_OWNER 		Object was already opened by another tool, will be
*							returned also if it applicable to one item in the family.
* OML_ERR_OBJ_CLOSED		Object was closed, and thus cannot be accessed at
*							this session any more. Only if all items under family.
*-------------------------------------------------------------------------------
*/
OML_EXPIMP GE_stat_t oml_obj_set_owner( GE_tool_id_t       tool_id, 
                                        OML_handle_t        obj_hndl,
                                        OML_owner_state_update_func_t state_update_clbk);
/*-------------------------------------------------------------------------------
* Name: oml_obj_owner_close
* =========================
* Description
* This function is used by the object owner to close the object, or family of
* objects, according to the obj_hndl. Closing an object is allowed until TC
* stage. The objects state is changed to "closed" after the closing was
* completed successfully. Notifier is always at "active" state, and it is an
* error attempting to close it.
* Closing an object that is already closed is ignored. Closing a defined object 
* will fail due to not owner problem.
*  
* Parameters:
* tool_id	- Used to identify the object owner.
* obj_hndl	- Used to identify the object to be closed.
*
* Return Values:
* None
* 
* Fatal Conditions
* OML_ERR_INVALID_GE_STAGE	Function was called in wrong OML stage.
* OML_ERR_INVALID_HNDL		Handle is not known to OML.
* OML_ERR_NOT_OWNER 		Object was already opened by another tool, will be
*							returned also if it applicable to one item in the family.
* OML_ERR_WRONG_CLASS		A notifier handle was given as a parameter.	
*-------------------------------------------------------------------------------
*/
OML_EXPIMP void oml_obj_owner_close(GE_tool_id_t       tool_id, 
                                    OML_handle_t       obj_hndl);

/*------------------------------------------------------------------------------
* Name: oml_obj_set_owner_info
* =============================
* Description
* This function is used by the owner to set its private information that should 
* be attached to the object, can be a pointer back to the own. The owner can
* change its content an any GE stage, but only after the object was opened. 
*
* Parameters:
* owner_id			The owner tool id.
* obj_hndl			Used to identify the object. Can specify an item only.
* owner_info_ptr	the owner information.
*
* Return Values:
* None
*					    Function worked correctly.
* Fatal Conditions:
* OML_ERR_OBJECT_NOT_OPEN	The object is defined or closed.
* OML_ERR_INVALID_HNDL		Object handle is not known to OML.
* OML_ERR_NOT_OWNER			The tool is not the owner of the given data object.
* OML_ERR_OBJ_CLOSED		Object was closed, and thus cannot be accessed at 
*							this session any more.
*-------------------------------------------------------------------------------
*/
OML_EXPIMP void oml_obj_set_owner_info( GE_tool_id_t    tool_id, 
                                        OML_handle_t    obj_hndl,
							            void		    *owner_info);
/*-------------------------------------------------------------------------------
* Name: oml_data_set_owner_val_clbk
* =================================
* Description:
* This function is used by the owner to register his callback function for the 
* data handling. If the data value is placed in the owner memory then these 
* functions must be given. If the data value is set only by the owner, then the
* set function might be null. In case that the data value is kept in OML, but
* the owner should make calculations before setting the data, then the owner
* will give a set function, and will set the calculated value using the value
* pointer of the data.
* This function can be called only after the data object is "opened" in TI GE
* stage. 
*
* Parameters:
* owner_id	The owner tool id.
* data_hndl	Used to identify the data. Can specify a data item only.
* get_val_clbk_ptr	The pointer to the callback function that will be called by
*					OML to get the data value.
* set_val_clbk_ptr	The pointer to the callback function that will be called by
*					OML to set the data value. 
* Both pointers can be null.
*
* Return Values:
* NONE
* 
* Fatal Conditions:
* OML_ERR_OBJ_NOT_OPENED	All items under the family are not opened.
*						(defined or closed)
* OML_ERR_INVALID_GE_STAGE	Function was called in wrong OML stage.
* OML_ERR_INVALID_HNDL	Handle is not known to OML.
* OML_ERR_NOT_OWNER	The tool is not the owner of the given data object.
* OML_ERR_WRONG_CLASS	Non data handle was given as a parameter.	
*-------------------------------------------------------------------------------
*/
							   
OML_EXPIMP void oml_data_set_owner_val_clbk(GE_tool_id_t         tool_id, 
                                            OML_handle_t            obj_hndl,
                                            OML_val_get_clbk_t   get_val_clbk_ptr,
                                            OML_val_set_clbk_t   set_val_clbk_ptr);

/*-------------------------------------------------------------------------------
* Name: oml_data_set_size
* =================================
* Description:
* This function is used by the owner to set the size of a byte array data item. 
* This function can be called only by the owner after the data object is "opened" 
* in TI GE stage. 
*
* Parameters:
* owner_id	The owner tool id.
* obj_hndl	Used to identify the data. Can specify a family of data items from 
*           byte_array type only.
* size		the size to set.
*
* Return Values:
* NONE
* 
* Fatal Conditions:
* OML_ERR_OBJ_NOT_OPENED	All items under the family are not opened.
*						(defined or closed)
* OML_ERR_INVALID_GE_STAGE	Function was called in wrong OML stage.
* OML_ERR_INVALID_HNDL	Handle is not known to OML.
* OML_ERR_NOT_OWNER	The tool is not the owner of the given data object.
* OML_ERR_WRONG_CLASS	Non data handle was given as a parameter.	
* OML_ERR_WRONG_TYPE	The data item is not of type byte array.
*-------------------------------------------------------------------------------
*/
							   
OML_EXPIMP void oml_data_set_size(GE_tool_id_t     tool_id, 
                       			  OML_handle_t     obj_hndl,
			                      unsigned		   size);
   
/***********************
*** CLIENT FUNCTIONS ***
************************/

OML_EXPIMP GE_stat_t oml_obj_request(GE_tool_id_t       tool_id, 
							     	 OML_handle_t       obj_hndl,
									 OML_client_receive_func_ptr_t client_receive_clbk);


OML_EXPIMP GE_stat_t oml_obj_request_with_cliet_info(
                        GE_tool_id_t       tool_id, 
						OML_handle_t       obj_hndl,
						OML_client_receive_func_ptr_with_client_info_t client_receive_clbk,
						void * client_info);


OML_EXPIMP GE_stat_t oml_obj_remove_request(GE_tool_id_t       tool_id, 
						  					OML_handle_t       obj_hndl,
										    OML_client_receive_func_ptr_t client_receive_clbk);

OML_EXPIMP GE_stat_t oml_obj_remove_request_with_cliet_info(
						GE_tool_id_t       tool_id, 
						OML_handle_t       obj_hndl,
						OML_client_receive_func_ptr_with_client_info_t client_receive_clbk,
						void * client_info);


/*-------------------------------------------------------------------------------
* Name:  oml_obj_require
* ======================
* Description
* This function is used by any tool that have the knowledge on the object
* requirement (can be the configure library or the client itself), to notify the
* owner on this need. If this is the first requirement of the object then OML
* will perform the object completeness checks to verify that the object can
* become available, from OML point of view. If the completeness checks are
* successful, then the owner is notified using his callback function
* state_update_clbk.
* Requiring an object is allowed only after it was opened and during the Tools 
* Configuration L1 stage. It is allowed to call this function many times by
* different tools. The object state will become "available" after the
* requirement was completed successfully. If the object is already at
* "available" or "active" state, then the requirement will be ignored.  
*
*
* Parameters:
* 
* obj_hndl	Used to identify the required object. For both a family or an item.
* tool_id	Used as conventional parameter.

* Return Values:
* GE_OK	                Function worked correctly.
* OML_ERR_OBJ_CLOSED	All objects in the family were closed,
*
* Fatal Conditions:
* OML_ERR_OBJECT_NOT_OPEN	All objects in the family are not opened. (defined)
* OML_ERR_INVALID_GE_STAGE	Function was called in wrong OML stage.
* OML_ERR_INVALID_HNDL	Object handle is not known to OML.
* OML_ERR_OBJ_NOT_READY	 complethness	checks failed.
* OML_ERR_OWNER_NOT_READY The owner cannot make it avilable 
*-----------------------------------------------------------------------------------
*/

OML_EXPIMP GE_stat_t oml_obj_require(   GE_tool_id_t     tool_id, 
        				                OML_handle_t      obj_hndl);


/*------------------------------------------------------------------------------
* Name: oml_obj_register
* ==================================
* Description:
* Function used for client registration for event or notifier notification.
* When the same tool is calling the register or request functions, 
* the new registration is ADDED to the client list. 
* This enables the same module to have multiple callback functions.
*
* Parameters:
* tool_id	The client tool ID.
* obj_hndl	The event or events class that client wish to be registered on.
* client_receive_clbk	Pointer to a callback function that will be called by
*						OML to notify the client on the event occurrence.
*
* Return Values:
* GE_OK	                Function worked correctly.
*
* Fatal Conditions:
* OML_ERR_NULL_FUNC	    Callback function pointer is null.
* OML_ERR_OBJ_NOT_OPENED	The event is not opened and thus registration in not
*						possible yet. 
*                       For a class of events this code will be returned only
*						if all events in the class are not available.
* OML_ERR_WRONG_CLASS	Non event/notifier handle was given as a parameter.
* OML_ERR_INVALID_HNDL	Handle is not known to OML.
*-------------------------------------------------------------------------------
*/

OML_EXPIMP GE_stat_t oml_obj_register(GE_tool_id_t       tool_id, 
							     	  OML_handle_t      obj_hndl,
									  OML_client_receive_func_ptr_t client_receive_clbk);

OML_EXPIMP GE_stat_t oml_obj_register_with_client_info(
                        GE_tool_id_t      tool_id, 
						OML_handle_t      obj_hndl,
						OML_client_receive_func_ptr_with_client_info_t client_receive_clbk,
                        void * client_info);

/*-------------------------------------------------------------------------------
* Name: oml_obj_unregister
* =================================
* Description :
* Client remove itself from the event's or notofier's registration list.
*
* Parameters:
* tool_id	The client tool ID.
* obj_hndl	The object handle that client wish to be cancel registration. 
*           Can specify an item or family.
* client_receive_clbk	Pointer to a callback function to be removed. NULL imply
*						removal of ALL registered callback of the same client.
*
* Return Values:
* GE_OK	                Function worked correctly.
* OML_ERR_OBJ_CLOSED	Object was closed, and thus cannot be accessed at this 
*                       session any more.	
* OML_ERR_CLIENT_NOT_REGISTER	The tool was not registered as a client of this 
*                               object, or the callback function was not given before.	
*
* Fatal Conditions:
* OML_ERR_WRONG_CLASS	Non event/notifier handle was given as a parameter.
* OML_ERR_INVALID_HNDL	Handle is not known to OML.
*-------------------------------------------------------------------------------
*/
OML_EXPIMP GE_stat_t oml_obj_unregister(GE_tool_id_t  tool_id, 
										OML_handle_t obj_hndl,
									    OML_client_receive_func_ptr_t client_receive_clbk);

OML_EXPIMP GE_stat_t oml_obj_unregister_with_client_info(
                        GE_tool_id_t      tool_id, 
						OML_handle_t      obj_hndl,
						OML_client_receive_func_ptr_with_client_info_t client_receive_clbk,
                        void * client_info);


/*-------------------------------------------------------------------------------
* Name: oml_obj_activate
* =============================
* Description
* Client declares that the object is needed as of now. First activation 
* changes the object state to "active". 
* Activation is allowed only for objects in "available" state. Activation is
* allowed only as of TC-L1 GE stage. If this is the first activation of the
* object, the object state will be changed to "active" and the owner will be
* updated.
* In case of an event activation, the tool must be a client of the event 
* (meaning registered the receive callback function). 
* This function is ignored if called for a notifier.
*
* Parameters:
* tool_id	The client tool ID.
* obj_hndl	The object handle that should be activated.
*
* Return Values:
* GE_OK	                Function worked correctly.
* OML_ERR_OBJ_CLOSED	Object is closed.
*
* Fatal Conditions:
* OML_ERR_OBJ_NOT_AVAILABLE	The object is not available and thus activation is  
*							not possible yet. For a family this code will be 
*                           returned only if all items in the family are not
*							available.
* OML_ERR_NOT_CLIENT	The tool is not registered as a client of the object
*						(meaningfull only to events). 
* OML_ERR_INVALID_GE_STAGE	Function was called in wrong OML stage.
* OML_ERR_INVALID_HNDL	Handle is not known to OML.
* OML_ERR_OWNER_NOT_READY	There were problems in updating the owner about state
*						change.
* OML_ERR_OUT_OF_MEMORY	Memory allocation failed, when try to add a client to
*						the clients list.
*-------------------------------------------------------------------------------
*/


OML_EXPIMP GE_stat_t oml_obj_activate(GE_tool_id_t tool_id, 
								  	  OML_handle_t obj_hndl);

/*-------------------------------------------------------------------------------
* Name: oml_obj_deactivate
* ===============================
* Description:
* The oml_obj_deactivate function is used by the clients to declare that 
* the object is not needed by it any more. De-activation is allowed only for 
* objects in "active" state, and is allowed only after TC-L1 GE stage. If this
* is the last de-activation of the object, the object state will be changed to
* "available" and the owner will be updated.Only clients that activated the
* object are allowed to deactivate it.
*
* Parameters:
* tool_id	The client tool ID.
* obj_hndl	The object handle that should be de-activated.
*
* Return Values:
* GE_OK	                Function worked correctly.
* OML_ERR_OBJ_NOT_ACTIVE	The object is not active and thus deactivation is not 
*                           possible yet. For a family of objects this code will 
*                           be returned only if all objects in the family are
*							not active.
* OML_ERR_CLIENT_NOT_ACTIVE	The client did not activated the object before.
* OML_ERR_OBJ_CLOSED	Object is closed
* 
* Fatal Conditions: 
* OML_ERR_INVALID_GE_STAGE	Function was called in wrong OML stage.
* OML_ERR_INVALID_HNDL	Handle is not known to OML.
* OML_ERR_OBJ_NOT_AVAILABLE	The object is not available and thus activation is 
*							not possible yet. For a family this code will be 
*                           returned only if all items in the family are not available.
* OML_ERR_OBJ_NOT_READY		There were problems in updating the owner about
*							state change.
*-------------------------------------------------------------------------------
*/
OML_EXPIMP GE_stat_t oml_obj_deactivate(GE_tool_id_t    tool_id, 
										OML_handle_t    obj_hndl);

/************************************
*** Object Manipulation Functions ***
*************************************/
/*-------------------------------------------------------------------------------
* Name:  oml_get_value 
* =============================
* Description:
* This function is used to retrieve the data object value. It can be used only 
* if the object is at "active" state. This function is applicable only for data
* items. OML will take the value owner's get function if such function exists,
* otherwise the internal OML data value is used. It is the client responsibility 
* to allocate enough memory for the value, if not than the buf_size_ptr 
* parameter will contain the needed size in bytes.
* In case of using the owner get function, OML will verify that the owner and
* OML are using the same size
* This function only do the basic checks and call oml_get_value for the "real" 
* work.
*
* Parameters:
* tool_id	The client tool ID.
* item_hndl	The item handle, must be a data handle.
* buf_size_ptr	Pointer to the value buffer size (in bytes). If the size given
*				to the buffer is too small or the buffer pointer is null, 
*				then the value size will be written into buf_size_ptr.
* val_buf_ptr	The item's value will be copied to the buffer, pointed by 
*				this parameter.
*
* Return Values:
* GE_OK	                Function worked correctly.	
* OML_ERR_OBJ_CLOSED	Object is closed, and thus cannot be accessed at this 
*						session any more.
* GE_ERR_BUFF_TOO_SHORT	Size given to the value buffer is too small. In this 
*						case the value siz will be written into buf_size_ptr, 
*						and the caller will be able to prepare the needed size 
*						and recall the function.
* OML_ERR_NULL_POINTER	The buffer pointer is NULL. In this case the value size 
*						will be written into buf_size_ptr, and the caller will 
*						be able to prepare the needed size and recall the function.
*
* Fatal Conditions
* OML_ERR_OBJ_NOT_ACTIVE	The object is not active 
* OML_ERR_NOT_ITEM	The object is not an item.
* OML_ERR_WRONG_CLASS	Non data handle was given as a parameter.
* OML_ERR_INVALID_HNDL	Handle is not known to OML.
* OML_ERR_WRONG_SIZE	The owner size and OML size are not the same.
*-------------------------------------------------------------------------------
*/

OML_EXPIMP GE_stat_t  oml_get_value(GE_tool_id_t tool_id, 
								    OML_handle_t item_hndl, 
								    unsigned     *buf_size_ptr, 
								    void		 *val_buf_ptr);
OML_EXPIMP GE_stat_t  oml_get_value_by_name(GE_tool_id_t tool_id, 
								    OML_name_t*   name_str,
								    unsigned     *buf_size_ptr, 
								    void		 *val_buf_ptr);

/*-------------------------------------------------------------------------------
* Name: oml_set_value 
* =============================
* Description:
* This function is used to set the data object value. It can be used only if 
* the object is at "available" or "active" states. This function is applicable
* only for objects data items.
* OML will use the owner's set function, if such function exists, otherwise the
* internal OML data value is set. OML will check that the given value size is
* appropriate to the size property of the object. In case of using the owner
* set function, OML will call the owner to do the setting and afterward will
* verify that the owner and OML are using the same size. 
* If the data value setting is allowed only to the owner, OML will check that 
* another tool does not attempt to set it.
*
* This function only do the basic checks and call oml_set_value for the "real" 
* work.
*
* Parameters:
* tool_id	The tool ID. Must be the owner in case of restricted data.
* item_hndl	The item handle, must be a data handle.
* buf_size_ptr	buf_size	The buffer size. Used by OML for safety checks.
* val_buf_ptr	The item's value to be set, pointed by this parameter.
*
* Return Values:
* GE_OK	                Function worked correctly.	
* GE_ERR_BUFF_TOO_SHORT	Size given to the value buffer is too small. In this 
*						case the value size will be written into buf_size_ptr, 
*						and the caller will be able to prepare 
*						the needed size and recall the function.
*
* Fatal Conditions
* OML_ERR_OBJ_NOT_AVAILABLE	The object is not available and thus setting its value is not 
*                           possible yet.
* OML_ERR_NOT_ITEM	The object is not an item.
* OML_ERR_OBJ_CLOSED	Object is closed.
* OML_ERR_WRONG_CLASS	Non data handle was given as a parameter.
* OML_ERR_INVALID_HNDL	Handle is not known to OML.
* OML_ERR_WRONG_SIZE	The owner size and OML size are not the same.
* OML_ERR_OUT_OF_MEMORY	Memory allocation failed.
* OML_ERR_NULL_POINTER	The buffer pointer is NULL.
* OML_ERR_NON_AUTHORIZED_TOOL	The object is restricted to owners update only,
*								and another  tool attempt to set it.
*-------------------------------------------------------------------------------
*/

OML_EXPIMP GE_stat_t oml_set_value(GE_tool_id_t  tool_id, 
							       OML_handle_t  item_hndl, 
							       unsigned      buf_size, 
							       void const    *val_buf_ptr);

OML_EXPIMP GE_stat_t oml_set_value_by_name(GE_tool_id_t  tool_id, 
								           OML_name_t*   name_str,
							       		   unsigned      buf_size, 
									       void const    *val_buf_ptr);


/*-------------------------------------------------------------------------------
* Name: oml_event_send
* =========================
* Description
* This function is used by the event owner to send the notification messages to
* all the registered clients. All registered clients will be called using their
* receive callback function. This function returns to the sender the success
* status, regarding the OML treatment for the message. There will be no
* identification on the clients' return status.
* Sending of event with no client is allowed. OML will check that the event
* sender is a legal owner, according to the event's properties.
* When sending the event, the owner transfer the control to OML, thus it must
* be in a situation that it can give any value of all his owned "active" data
* objects. The sending algorithm is described in oml_send_message function.
*
*
* Parameters:
* owner_id	The owner tool ID.
* event_hndl The event that occurred. The handle must specify an event item.
*
* Return Values:
* GE_OK	                Function worked correctly.
* OML_ERR_OBJ_UNDER_TREATMENT	Repeated send of the event, before all its clients were informed on the previous send.
*
* Fatal Conditions
* OML_ERR_NOT_ITEM	A family handle was given instead of an item handle.
* OML_ERR_INVALID_SEQUENCE	The event is not obeying the legal sequences, 
*                           as described in oml_send_message.
* OML_ERR_NOT_OWNER	The tool id is not belong to the event owner.
* OML_ERR_OBJ_NOT_OPENED	The event was not opened, and thus 
*						sending it in not possible.
* OML_ERR_OBJ_CLOSED	Object is closed, and thus cannot be accessed at 
*						this session any more.
* OML_ERR_WRONG_CLASS	Non event handle was given as a parameter.
* OML_ERR_INVALID_HNDL	Event handle is not known to OML.
*-------------------------------------------------------------------------------
*/
OML_EXPIMP GE_stat_t  oml_event_send(GE_tool_id_t tool_id, 
							         OML_handle_t item_hndl, ...);

OML_EXPIMP GE_stat_t  oml_event_send_by_name(GE_tool_id_t tool_id, 
								             OML_name_t*   name_str, ...);

/*-------------------------------------------------------------------------------
* Name: oml_obj_get_property
* ==========================
* Description
* The oml_obj_get_property function is used to query the object's property.
* The caller is responsible to allocate the memory needed for the property.
* 
* Using this function is allowed for all objects states, but some of the
* properties might be empty if the object was not opened yet.

* Parameters:
* tool_id	The tool asking for the property.
* obj_hndl	Used to identify the object.
* property_id	The requested property. 
* *prop_buf_ptr	Address to the buffer that will be filled by OML with the 
*				requested property. The buffer is of void type and its size 
*				should be appropriate according to the property size. Property 
*				sizes described in omlapi.h. If the buffer pointer is null 
*				it means that the caller is interested only in the property size.
* *prop_buf_size_ptr	Pointer to the property buffer size. If the size given 
*						to the buffer is too small or the buffer pointer is 
*						null, then the property size will be written into 
*						property_buf_size. This parameter is important mainly 
*						in cases of strings or a structures, where the size is 
*						not predefined.
*
* Return Values:
* GE_OK	                Function worked correctly.
* GE_ERR_BUFF_TOO_SHORT	Size given to a string buffer is too small. In this 
*						case the property size will be written into 
*						property_buf_size, and the caller will be able to 
*						prepare the needed size and recall the function.
* 
* Fatal Conditions
* OML_ERR_NOT_ITEM	Family handle was given to a non family property.
* OML_ERR_NOT_OWNER	Value pointer or owner information were asked, 
*					not by the owner.
* OML_ERR_STRING_ADDR	The string value address was requested
* OML_ERR_INVALID_HNDL	Object handle is not known to OML.
* OML_ERR_INVALID_PROP	property_id is not known to OML.
*-------------------------------------------------------------------------------
*/
OML_EXPIMP GE_stat_t  oml_obj_get_property(GE_tool_id_t      tool_id, 
								OML_handle_t      obj_hndl,
								OML_obj_prop_t    property_id,
								void              *prop_buf_ptr,
								unsigned          *prop_buf_size_ptr);


OML_EXPIMP GE_stat_t oml_check_hndl_4_get(GE_tool_id_t  tool_id,
				     		              OML_handle_t  item_hndl,
				     		              unsigned      size);

OML_EXPIMP GE_stat_t oml_check_hndl_4_set(GE_tool_id_t  tool_id,
				     		              OML_handle_t  item_hndl,
				     		              unsigned      size);

OML_EXPIMP GE_stat_t oml_check_hndl_4_send(GE_tool_id_t  tool_id,
				     		               OML_handle_t  item_hndl);



/****************************************************************
 ****** old API functions still supported for compatability  ****
 ****************************************************************/

OML_EXPIMP GE_stat_t oml_event_client_register(GE_tool_id_t          tool_id, 
									OML_handle_t      obj_hndl,
									OML_client_receive_func_ptr_t client_receive_clbk);

OML_EXPIMP GE_stat_t oml_event_client_register_with_client_info(
				   GE_tool_id_t          tool_id, 
				   OML_handle_t      obj_hndl,
				   OML_client_receive_func_ptr_with_client_info_t client_receive_clbk,
				   void * client_info);


OML_EXPIMP GE_stat_t oml_notifier_client_register(GE_tool_id_t       tool_id, 
									OML_handle_t      obj_hndl,
									OML_client_receive_func_ptr_t client_receive_clbk);


OML_EXPIMP GE_stat_t oml_notifier_client_register_with_client_info(GE_tool_id_t          tool_id, 
						OML_handle_t      obj_hndl,
						OML_client_receive_func_ptr_with_client_info_t client_receive_clbk,
						void * client_info);


OML_EXPIMP GE_stat_t oml_event_client_unregister(GE_tool_id_t  tool_id, 
										OML_handle_t obj_hndl);

OML_EXPIMP GE_stat_t oml_notifier_client_unregister(GE_tool_id_t     tool_id, 
										  OML_handle_t obj_hndl);

OML_EXPIMP GE_stat_t oml_obj_client_activate(GE_tool_id_t tool_id, 
								  OML_handle_t obj_hndl);

OML_EXPIMP GE_stat_t oml_obj_client_deactivate(GE_tool_id_t    tool_id, 
									OML_handle_t    obj_hndl);

OML_EXPIMP GE_stat_t oml_data_item_get_value(GE_tool_id_t tool_id, 
								  OML_handle_t item_hndl, 
								  unsigned     *buf_size_ptr, 
								  void		   *val_buf_ptr);

OML_EXPIMP GE_stat_t oml_notifier_item_get_value(GE_tool_id_t tool_id, 
									  OML_handle_t item_hndl, 
									  unsigned     *buf_size_ptr, 
									  void         *val_buf_ptr);

OML_EXPIMP GE_stat_t oml_data_item_set_value(GE_tool_id_t tool_id, 
							 OML_handle_t  item_hndl, 
							 unsigned      buf_size, 
							 void const    *val_buf_ptr);

OML_EXPIMP GE_stat_t oml_notifier_item_set_value(GE_tool_id_t tool_id, 
							 OML_handle_t    item_hndl, 
							 unsigned        buf_size, 
							 void const      *val_buf_ptr);

OML_EXPIMP GE_stat_t  oml_event_item_send(GE_tool_id_t tool_id, 
										  OML_handle_t item_hndl);

OML_EXPIMP OML_handle_t oml_add_item(char* name, 
									GE_value_type_t type, 
									unsigned size,
									GE_tool_id_t tool_id);

OML_EXPIMP void oml_set_fatal_error_function(OML_fatal_error_t fatal_error);

OML_EXPIMP void oml_set_usr_message_function(OML_usr_message_t usr_message);

OML_EXPIMP void oml_set_tool_name_function(OML_tool_name_t tool_name);


int vpc_strtok
( char * input_str,//(IN) string to parse
  char * sep,//(IN) the seperators array
  char * token,//(out) the token
  void * hndl// (IN/OUT) -pointer to last found token
); 

#ifdef __cplusplus
};
#endif

#endif /**** OMLAPI_H ****/
