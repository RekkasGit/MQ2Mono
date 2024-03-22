# MQ2Mono

One Paragraph project description goes here

## Getting Started

Quick start instructions to get users up and going

```txt
/plugin MQ2Mono
```

### Commands

list, version, unload, stop, load, run, start, 

```txt
/mono list
/mono load e3
/mono e3
/mono unload e3
```

### TLO
The TLO method Query allows you to send an OnQuery command to a Mono application running. Its up to that application
on how it will respond, but it will be in a string format. You specify the application as the 1st param. Its the same name as the primary *.dll you loaded on 
```txt
/mono load <applicationname> 
or
/mono <applicationname>
```
```txt
/echo ${MQ2Mono}
/echo ${MQ2Mono.Query[e3,InCombat]}
/echo ${MQ2Mono.Query[e3,E3Bots(Necro01).Query(ShowClass)]}
```
### Installation

<ul>
  <li>Place the Directory: Ensure that you place the directory in the Macroquest/plugins folder. Do not place it in Macroquest/src/plugins. </li>
<li>Add the Plugin: Add the plugin to your 'macroquest' solution. </li>
<li>Build MQ2Mono: Compile the MQ2Mono component.</li>
<li>Copy the DLL:  Copy the mono-2.0-sgen.dll (32bit/64bit) file into the appropriate build directory. <br/>
This will be either build/bin/release/ or build/bin/debug/, depending on your build configuration.<br/>
Make sure the macroquest.exe is present in the directory.<br/>
</li>
</ul>

### Authors

**Rekka** - *Initial work*

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

