import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    visible: true
    width: 1380; height: 900
    minimumWidth: 960; minimumHeight: 650
    title: "ChronOS — Scheduling Laboratory"
    color: "#0c1422"
    palette.window: "#0c1422"; palette.windowText: "#dce7f5"
    palette.base: "#131f32"; palette.text: "#dce7f5"
    palette.button: "#213550"; palette.buttonText: "#dce7f5"
    palette.highlight: "#42cfbd"; palette.highlightedText: "#0c1422"
    property var data: controller.report
    property var metrics: data.metrics || ({})
    property var config: ({tasks:[]})
    property int page: 0
    property var comparisons: []
    property int editedTask: -1
    function refreshConfig() { try { config = JSON.parse(controller.scenarioText) } catch(e) {} }
    function pretty(value) { return JSON.stringify(value, null, 2) }
    function number(value) { return Number(value || 0).toFixed(2) }
    Component.onCompleted: refreshConfig()
    Connections {
        target: controller
        function onScenarioTextChanged() { root.refreshConfig(); scenarioEditor.text = controller.scenarioText }
        function onComparisonReady(json) { root.comparisons = JSON.parse(json) }
    }
    FileDialog { id: loadDialog; title: "Load scenario"; nameFilters: ["JSON scenarios (*.json)"]; onAccepted: controller.load(selectedFile) }
    FileDialog { id: saveDialog; title: "Save scenario"; fileMode: FileDialog.SaveFile; nameFilters: ["JSON scenarios (*.json)"]; onAccepted: controller.save(selectedFile) }
    FolderDialog { id: exportDialog; title: "Choose a new empty report directory"; onAccepted: controller.exportTo(selectedFolder) }
    header: ToolBar {
        RowLayout {
            anchors.fill: parent; anchors.margins: 10; spacing: 10
            Label { text: "CHRONOS"; font.pixelSize: 22; font.bold: true; color: "#42cfbd" }
            Label { text: "OS CONCEPTS / SINGLE CPU"; color: "#8d9fb9"; Layout.fillWidth: true }
            Label { text: "t = " + (root.data.time || 0) + " ticks"; font.family: "Consolas" }
            Label { text: controller.running ? "RUNNING" : root.data.finished ? "FINISHED" : "PAUSED"; color: "#f2bf65" }
        }
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 18; spacing: 12
        Flow {
            Layout.fillWidth: true; implicitHeight: childrenRect.height; spacing: 6
            Button { text: "Start / Resume"; onClicked: controller.start() }
            Button { text: "Pause / Stop"; onClicked: controller.pause() }
            Button { text: "Reset"; onClicked: controller.reset() }
            Button { text: "Step event batch"; onClicked: controller.step() }
            Button { text: "+1 tick"; onClicked: controller.tick() }
            ComboBox {
                model: ["fcfs","sjf","srtf","rr","fixed","rms","edf"]
                currentIndex: Math.max(0, model.indexOf(root.config.scheduler || "fixed"))
                onActivated: controller.changePolicy(currentText)
            }
            Label { text: "Playback"; height: 40; verticalAlignment: Text.AlignVCenter }
            Slider { from: 0.25; to: 16; value: 1; width: 110; onMoved: controller.setSpeed(value) }
            Button { text: "Load"; onClicked: loadDialog.open() }
            Button { text: "Save"; onClicked: saveDialog.open() }
            Button { text: "Export"; onClicked: exportDialog.open() }
        }
        Label { visible: controller.error.length > 0; text: controller.error; color: "#ff8f8f"; wrapMode: Text.Wrap; Layout.fillWidth: true }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 18
            Rectangle {
                Layout.preferredWidth: 180; Layout.fillHeight: true; color: "#121f32"; radius: 8
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 10
                    Repeater {
                        model: ["Dashboard","Scheduler Lab","Task Manager","Resource Monitor","Interrupt Lab","Deadlock Analyzer","Analytics","Scenario Library","Settings"]
                        delegate: Button {
                            required property string modelData
                            required property int index
                            text: modelData; flat: root.page !== index
                            Layout.fillWidth: true; onClicked: root.page = index
                        }
                    }
                    Item { Layout.fillHeight: true }
                    Label { text: "C++20 · Qt 6\nInteger virtual time\nUnverified build"; color: "#8d9fb9"; font.pixelSize: 12 }
                }
            }
            StackLayout {
                currentIndex: root.page; Layout.fillWidth: true; Layout.fillHeight: true
                ColumnLayout {
                    Label { text: root.config.name || "Scheduling laboratory"; font.pixelSize: 26; font.bold: true }
                    Label { text: "Every displayed result comes from the engine."; color: "#8d9fb9" }
                    GridLayout {
                        columns: 4; Layout.fillWidth: true
                        Repeater {
                            model: [
                                ["CPU utilization",root.number(root.metrics.cpu_utilization*100)+"%"],
                                ["Completed jobs",String(root.metrics.completed || 0)],
                                ["Missed deadlines",String(root.metrics.deadline_misses || 0)],
                                ["Context switches",String(root.metrics.context_switches || 0)],
                                ["Ready wait / completed",root.number(root.metrics.average_waiting)],
                                ["Response / dispatched",root.number(root.metrics.average_response)],
                                ["Resource blocking",String(root.metrics.total_blocking || 0)],
                                ["IRQ service ticks",String(root.metrics.interrupt_time || 0)]
                            ]
                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true; implicitHeight: 85; radius: 6; color: "#17263c"
                                Column { anchors.fill: parent; anchors.margins: 10; spacing: 7
                                    Label { text: modelData[0]; color: "#8d9fb9"; font.pixelSize: 10 }
                                    Label { text: modelData[1]; font.pixelSize: 24; color: "#54dfcf" }
                                }
                            }
                        }
                    }
                    Timeline { report: root.data; Layout.fillWidth: true; Layout.fillHeight: true }
                    Label { text: root.data.analysis ? root.data.analysis.result : ""; color: "#f2bf65"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                }
                ColumnLayout {
                    Label { text: "Scheduler Lab"; font.pixelSize: 26 }
                    Label { text: "Algorithm changes reset the run. Higher fixed priority numbers win."; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Timeline { report: root.data; Layout.fillWidth: true; Layout.fillHeight: true }
                    ScrollView { Layout.fillWidth: true; Layout.preferredHeight: 180
                        TextArea { readOnly: true; text: root.pretty(root.data.jobs || []); font.family: "Consolas"; font.pixelSize: 12 }
                    }
                }
                ColumnLayout {
                    Label { text: "Task Manager"; font.pixelSize: 26 }
                    Label { text: "Edits validate and reset the run. Edit resource actions in Scenario Library."; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    ComboBox {
                        Layout.fillWidth: true; model: root.config.tasks || []; textRole: "id"
                        onActivated: {
                            root.editedTask = currentIndex
                            let t = root.config.tasks[currentIndex]
                            taskId.text=t.id; execution.text=String(t.execution); period.text=String(t.period || 0)
                            deadline.text=String(t.deadline); release.text=String(t.release || 0); priorityValue.text=String(t.priority || 0)
                        }
                    }
                    GridLayout {
                        columns: 4; Layout.fillWidth: true
                        Label { text: "Task ID" } TextField { id: taskId; text: "NewTask"; Layout.fillWidth: true }
                        Label { text: "Execution" } TextField { id: execution; text: "2"; validator: IntValidator { bottom: 1; top: 1000000000 } }
                        Label { text: "Period (0=aperiodic)" } TextField { id: period; text: "0"; validator: IntValidator { bottom: 0; top: 1000000000 } }
                        Label { text: "Relative deadline" } TextField { id: deadline; text: "10"; validator: IntValidator { bottom: 1; top: 1000000000 } }
                        Label { text: "Release" } TextField { id: release; text: "0"; validator: IntValidator { bottom: 0; top: 1000000000 } }
                        Label { text: "Base priority" } TextField { id: priorityValue; text: "1"; validator: IntValidator { bottom: -1000000; top: 1000000 } }
                    }
                    RowLayout {
                        Button { text: "Add"; onClicked: controller.upsertTask({id:taskId.text,name:taskId.text,execution:Number(execution.text),period:Number(period.text),deadline:Number(deadline.text),release:Number(release.text),priority:Number(priorityValue.text)},-1) }
                        Button { text: "Update"; enabled: root.editedTask >= 0; onClicked: controller.upsertTask({id:taskId.text,execution:Number(execution.text),period:Number(period.text),deadline:Number(deadline.text),release:Number(release.text),priority:Number(priorityValue.text)},root.editedTask) }
                        Button { text: "Delete"; enabled: root.editedTask >= 0; onClicked: { controller.removeTask(root.editedTask); root.editedTask=-1 } }
                    }
                    ScrollView { Layout.fillWidth: true; Layout.fillHeight: true
                        TextArea { readOnly: true; text: root.pretty(root.config.tasks); font.family: "Consolas" }
                    }
                }
                ColumnLayout {
                    Label { text: "Resource Monitor"; font.pixelSize: 26 }
                    Label { text: "Owner / FIFO waiters · priority inheritance " + (root.config.priority_inheritance ? "ON" : "OFF"); color: "#54dfcf" }
                    ScrollView { Layout.fillWidth: true; Layout.fillHeight: true
                        TextArea { readOnly:true; text: root.pretty({resources:root.data.resources || [],jobs:(root.data.jobs || []).map(j=>({job:j.id,task:j.task,state:j.state,base:j.base_priority,effective:j.effective_priority,blocking:j.blocking}))}); font.family:"Consolas" }
                    }
                }
                ColumnLayout {
                    Label { text: "Interrupt Lab"; font.pixelSize: 26 }
                    Label { text: "Latency = first service − arrival. Masks defer new handlers."; wrapMode: Text.Wrap; Layout.fillWidth:true }
                    Timeline { report:root.data; Layout.fillWidth:true; Layout.preferredHeight:230 }
                    ScrollView { Layout.fillWidth:true; Layout.fillHeight:true
                        TextArea { readOnly:true; text:root.pretty(root.data.interrupts || []); font.family:"Consolas" }
                    }
                }
                ColumnLayout {
                    Label { text: "Deadlock Analyzer"; font.pixelSize: 26 }
                    Label { text: "Arrows: waiter → owner. Historical cycles stay recorded."; color:"#8d9fb9" }
                    WaitGraph { report: root.data; Layout.fillWidth:true; Layout.fillHeight:true }
                    ScrollView { Layout.fillWidth:true; Layout.preferredHeight:140
                        TextArea { readOnly:true; text:root.pretty(root.data.deadlocks || []); font.family:"Consolas" }
                    }
                }
                ColumnLayout {
                    Label { text:"Analytics & Comparison"; font.pixelSize:26 }
                    Button { text:"Compare all policies with this workload"; onClicked:controller.comparePolicies() }
                    Label { text:"Unsupported policies report constraints. Workload fields stay identical."; wrapMode:Text.Wrap; Layout.fillWidth:true; color:"#8d9fb9" }
                    ScrollView { Layout.fillWidth:true; Layout.preferredHeight:290
                        Row { spacing:12
                            Repeater { model:root.comparisons
                                delegate: Column { required property var modelData
                                    width:290; spacing:4
                                    Label { text:modelData.scheduler.toUpperCase() }
                                    Rectangle { width:110; height:8; color:"#213550"
                                        Rectangle { width:parent.width*(modelData.metrics ? modelData.metrics.cpu_utilization : 0); height:8; color:"#54dfcf" }
                                    }
                                    Label { text:modelData.metrics ? "Misses: "+modelData.metrics.deadline_misses : "Unsupported"; font.pixelSize:11 }
                                    Label { text:modelData.metrics ? "Wait: "+root.number(modelData.metrics.average_waiting) : ""; font.pixelSize:11 }
                                    Label { text:modelData.metrics ? "Response: "+root.number(modelData.metrics.average_response)+" / Switches: "+modelData.metrics.context_switches : modelData.unsupported; font.pixelSize:11; width:280; wrapMode:Text.Wrap }
                                    Timeline { visible:!!modelData.metrics; width:280; height:190; report:modelData }
                                }
                            }
                        }
                    }
                    ScrollView { Layout.fillWidth:true; Layout.fillHeight:true
                        TextArea { readOnly:true; text:root.pretty(root.comparisons.length ? root.comparisons : {metrics:root.metrics,analysis:root.data.analysis}); font.family:"Consolas"; font.pixelSize:12 }
                    }
                }
                ColumnLayout {
                    Label { text:"Scenario Library"; font.pixelSize:26 }
                    RowLayout {
                        ComboBox { id: library; model:controller.scenarios(); textRole:"name"; Layout.fillWidth:true }
                        Button { text:"Load selected"; enabled:library.count>0; onClicked:controller.load(library.model[library.currentIndex].url) }
                        Button { text:"Apply JSON / Reset"; onClicked:{controller.scenarioText=scenarioEditor.text;controller.applyScenario()} }
                    }
                    Label { text:"Edit task actions, interrupts, masks, watchdog, quantum and horizon."; wrapMode:Text.Wrap; Layout.fillWidth:true; color:"#8d9fb9" }
                    ScrollView { Layout.fillWidth:true; Layout.fillHeight:true
                        TextArea { id:scenarioEditor; text:controller.scenarioText; selectByMouse:true; font.family:"Consolas"; font.pixelSize:13 }
                    }
                }
                ColumnLayout {
                    Label { text:"Settings & Model"; font.pixelSize:26 }
                    Label { text:"Single CPU · integer virtual ticks · zero switch overhead\nFIFO mutex wake-up · higher number = higher fixed priority\nTies: stable ready order, then job ID\nDeadline misses do not terminate jobs\nStop pauses; Reset discards runtime history\nPlayback affects presentation delay only\nUI: last 2,000 events and 5,000 intervals; export: full trace\nExports: summary/scenario JSON, CSV tables, SVG timeline\nRTOS concepts simulator, not a production kernel."; lineHeight:1.5; wrapMode:Text.Wrap; Layout.fillWidth:true }
                    Label { text:"Not compiled or runtime-tested in this environment."; color:"#f2bf65"; wrapMode:Text.Wrap; Layout.fillWidth:true }
                    Item { Layout.fillHeight:true }
                }
            }
        }
        Label { text:"Trace events: "+(root.data.event_count || 0)+"    ·    "+(root.config.description || ""); color:"#8d9fb9"; elide:Text.ElideRight; Layout.fillWidth:true }
    }
}
