Version: 1.8.1 (September 15, 2012)

The file JRegistry-1.8.1_src-bin_x86__x86-64.zip contains
the source and binaries for both 32-bit and 64-bit operating systems.

Changes in this release include:
  --Fixed RegistryQuota compareTo method always returns 0.
  --Fixed various bugs using the FindBugs tool.
  --Changed the RegistryKey serialVersionUID.
  --Corrected various JavaDoc errors.

Version: 1.8.0 (August 11, 2011)

The file JRegistry-1.8.0_src-bin_x86__x86-64.zip contains
the source and binaries for both 32-bit and 64-bit operating systems.

Changes in this release include:
 --Added static connect(String, RegistryKey) method to support remote registry connection.
 --Added isRemoteRegistryKey() to test if a key is from a remote registry.
 --Added RegistryQuota class to receive the system registry current size and limit.
 --Added an iterator to get subkeys and subkey names.
 --Added an iterator to get values and value names.
 --Added deleteView64Key(String) to delete a subkey from a specific view of the registry.
 --Added support for Windows Vista RegDeleteTree.
 --Added deleteSubTree(String) to a delete a full subtree starting at the specified subkey.
 --Added support for Windows Vista RegCopyTree.
 --Added copySubTree(String, RegistryKey) to copy a full subtree starting at the specified subkey.
 --Added deleteSubKeyValue(String, String) and deleteSubKeyValue(String, RegistryValue)
     to support RegDeleteKeyValue for Windows Vista. The above methods will also work for Windows 2000 and XP.
 --Added saveKey(boolean, boolean, File) to support RegSaveKeyEx
 --Added renameValue(String, String)
 --Fixed renameValue implementation so that a possible rename doesn't result in a copy
     For example, if key A holds value B and key X does renameValue(B,n), then that results in a copy of
     B being made in X.
 --Added copyValues(RegistryKey, String...) to copy values to a key.
 --Added copyValue(RegistryKey, RegistryValue) to copy one value to a key.
 --Added copyValues(RegistryKey, RegistryValue...) to copy values to a key.
 --Added copySubKeyValues(String, RegistryKey, String...) to copy values from a subkey to another key.
 --Added copySubKeyValue(String, RegistryKey, RegistryValue) to copy one value from a subkey to another key.
 --Added copySubKeyValues(String, RegistryKey, RegistryValue...) to copy values from a subkey to another key.
 --Added deleteSubKeys(String...) and deleteView64Keys(String...).
 --Added deleteValues(String...), deleteValues(RegistryValue...), deleteSubKeyValues(String, String...),
     and deleteSubKeyValues(String, RegistryValue...).
 --Added getSubKeys(String...) and support for RegQueryMultipleValues via getValues(String...).
 --Added boolean option to copy and delete physical link keys.
 --Added static methods for deleteKeys(RegistryKey...) and deleteLinks(RegistryKey...).
 --Added a method to convert HKEY_*_INDEX values to a RegistryKey: getRootKetForIndex(int).
 --Fixed the setLinkTo(RegistryKey) method which included a small error.
 --Fixed all methods to properly update the lastError code whenever possible.
 --Corrected various JavaDoc errors.

Version: 1.7.6 (May 26, 2011)

The file JRegistry-1.7.6_src-bin_x86__x86-64.zip contains
the source and binaries for both 32-bit and 64-bit operating systems.

Changes in this release include:
 --Using variable arguments in the RegistryValue, RegBinaryValue, and RegMultiStringValue classes.
 --Allowing the use of the java.library.path system property to specify the location
   of the JRegistry binary which should be loaded.