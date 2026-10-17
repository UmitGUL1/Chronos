import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: chart
    property var report: ({})
    property real zoom: 1
    property var selected: null
    property string selectedDetails: ""
    property real rangeStart: -1
    property real rangeEnd: -1
    property real observedEnd: Math.max(1, report.time || 1)
    function colorFor(i) {
        let colors=["#54dfcf","#6eb0ff","#bb9af7","#ecc479","#ef8fad"]
        return i.kind === "idle" ? "#28374d" : i.kind === "interrupt" ? "#bb9af7" : colors[(i.job || 0)%colors.length]
    }
    onReportChanged: { selectedDetails=""; if(canvas)canvas.requestPaint() }
    RowLayout {
        Label { text:"CPU 0 · Gantt trace"; font.bold:true }
        Item { Layout.fillWidth:true }
        Label { text:"Zoom" }
        Slider { id:zoomSlider; from:1; to:10; value:1; Layout.preferredWidth:100; onMoved:{chart.zoom=value;canvas.requestPaint()} }
        Button { text:"Reset view"; onClicked:{zoomSlider.value=1;chart.zoom=1;scroll.contentX=0;chart.rangeStart=-1;chart.rangeEnd=-1;canvas.requestPaint()} }
    }
    Flickable {
        id:scroll; clip:true; Layout.fillWidth:true; Layout.fillHeight:true
        contentWidth:Math.max(width,width*chart.zoom); contentHeight:canvas.height
        ScrollBar.horizontal:ScrollBar {}
        Canvas {
            id:canvas; width:scroll.contentWidth; height:Math.max(170,scroll.height)
            onWidthChanged:requestPaint()
            onHeightChanged:requestPaint()
            onPaint:{
                let c=getContext("2d");c.reset();c.fillStyle="#121f32";c.fillRect(0,0,width,height)
                let sx=(width-30)/chart.observedEnd
                c.font="11px sans-serif";c.fillStyle="#8d9fb9"
                for(let k=0;k<=10;k++){
                    let t=chart.observedEnd*k/10;let x=15+t*sx;c.fillText(t.toFixed(1),x,22)
                    c.strokeStyle="#25354b";c.beginPath();c.moveTo(x,30);c.lineTo(x,130);c.stroke()
                }
                for(let i of (chart.report.timeline || [])){
                    let x=15+i.begin*sx;let w=(i.end-i.begin)*sx
                    c.fillStyle=chart.colorFor(i);c.fillRect(x,44,Math.max(1,w-1),48)
                    if(w>40){c.fillStyle="#101827";c.fillText(i.label,x+4,71)}
                }
                // Deadline markers include on-time deadlines, not only misses.
                for(let j of (chart.report.jobs || [])){
                    if(j.deadline>chart.observedEnd)continue
                    let x=15+j.deadline*sx;c.strokeStyle=j.missed?"#ff7373":"#61758d"
                    c.beginPath();c.moveTo(x,34);c.lineTo(x,105);c.stroke()
                }
                for(let e of (chart.report.events || [])){
                    if(e.kind!=="preempt")continue
                    let x=15+e.time*sx;c.strokeStyle="#f2bf65";c.beginPath();c.moveTo(x,34);c.lineTo(x,104);c.stroke()
                }
                let blocked={};c.fillStyle="#f2bf65"
                for(let e of (chart.report.events || [])){
                    if(e.kind==="block")blocked[e.job]=e.time
                    if(e.kind==="wake" && blocked[e.job]!==undefined){
                        c.fillRect(15+blocked[e.job]*sx,115,Math.max(1,(e.time-blocked[e.job])*sx),4);delete blocked[e.job]
                    }
                }
                for(let id in blocked)c.fillRect(15+blocked[id]*sx,115,Math.max(1,(chart.observedEnd-blocked[id])*sx),4)
                if(chart.rangeStart>=0 && chart.rangeEnd>=0){
                    c.fillStyle="#ffffff20";let a=Math.min(chart.rangeStart,chart.rangeEnd);let b=Math.max(chart.rangeStart,chart.rangeEnd)
                    c.fillRect(15+a*sx,30,(b-a)*sx,100)
                }
            }
            MouseArea {
                anchors.fill:parent; hoverEnabled:true; acceptedButtons:Qt.LeftButton
                onPositionChanged:function(mouse){
                    let time=(mouse.x-15)*chart.observedEnd/(canvas.width-30);chart.selected=null
                    for(let i of (chart.report.timeline || []))if(i.begin<=time && time<i.end){chart.selected=i;break}
                    if(pressed){chart.rangeEnd=Math.min(chart.observedEnd,Math.max(0,time));canvas.requestPaint()}
                }
                onPressed:function(mouse){chart.rangeStart=Math.min(chart.observedEnd,Math.max(0,(mouse.x-15)*chart.observedEnd/(canvas.width-30)));chart.rangeEnd=chart.rangeStart;canvas.requestPaint()}
                onClicked: {
                    if(chart.selected && chart.selected.job!==null) {
                        for(let j of (chart.report.jobs || []))if(j.id===chart.selected.job){chart.selectedDetails="Job "+j.id+" / "+j.task+" / "+j.state+" / service "+j.executed+" / ready wait "+j.waiting+" / blocked "+j.blocking;break}
                    }
                }
                onExited:chart.selected=null
            }
        }
    }
    Label {
        text:chart.selected ? chart.selected.label+" · ["+chart.selected.begin+", "+chart.selected.end+") · "+chart.selected.kind : "Hover/select a job interval. Drag to select a range; use scrollbar to pan."
        color:"#8d9fb9"; wrapMode:Text.Wrap; Layout.fillWidth:true
    }
    Label {
        visible:chart.rangeStart>=0 && chart.rangeEnd>=0
        text:"Selected: "+Math.min(chart.rangeStart,chart.rangeEnd).toFixed(2)+" – "+Math.max(chart.rangeStart,chart.rangeEnd).toFixed(2)+" ticks"
        color:"#f2bf65"
    }
    Label { visible:chart.selectedDetails.length>0; text:chart.selectedDetails; color:"#54dfcf"; wrapMode:Text.Wrap; Layout.fillWidth:true }
}
