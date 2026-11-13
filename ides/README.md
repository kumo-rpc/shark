# IDE 插件

`.shark` 仍使用现有 proto 语法。两边插件只改了文件关联和显示名。

## VS Code

目录：`ides/vscode`。

```bash
cd ides/vscode
npm install
npm run package:vsix
```

产物：`ides/vscode/shark-<version>.vsix`（当前 `shark-0.5.10.vsix`）。插件自带 `include/shark/descriptor.shark` 和 `include/shark/types/*.shark`，编译时自动加上这份 `--proto_path`。

安装：Extensions 视图 → `…` → **Install from VSIX…**，选该文件。改图标或版本号后需要重新打包并再装一次。

`vsce` 用仓库里 `node_modules` 里的那份。不要从仓库根目录跑 `npx vsce`。

## CLion

目录：`ides/clion`。面向 CLion / IntelliJ 平台，只打包编辑器核心（高亮、补全、导航）。Java / Go / Python 跳转模块不进安装包，CLion 里没有对应依赖，带上会导致整包加载失败。

本机没有单独的 JDK 时，用 CLion 自带的 JBR。Gradle 9.1 才能在这套 JDK 25 上跑。

```bash
cd ides/clion
export JAVA_HOME="/Applications/CLion.app/Contents/jbr/Contents/Home"
export PATH="$JAVA_HOME/bin:$PATH"
./gradlew buildPlugin --no-daemon
```

产物：`ides/clion/build/distributions/clion-0.5.10.zip`。内置描述符是 `shark/descriptor.shark`，well-known 类型在 `shark/types/`。

安装：**Settings → Plugins → 齿轮 → Install Plugin from Disk…**，选该 zip，然后重启。重启后 `.shark` 的文件类型应为 Protocol Buffer。
