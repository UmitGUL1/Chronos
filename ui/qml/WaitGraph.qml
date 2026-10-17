import QtQuick

Canvas {
    id:graph
    property var report: ({})
    onReportChanged:requestPaint()
    onWidthChanged:requestPaint()
    onHeightChanged:requestPaint()
    onPaint:{
        let c=getContext("2d");c.reset();c.fillStyle="#121f32";c.fillRect(0,0,width,height)
        let resources=report.resources || [];let ids=[]
        for(let r of resources){if(r.owner!==null && ids.indexOf(r.owner)<0)ids.push(r.owner);for(let w of r.waiting)if(ids.indexOf(w)<0)ids.push(w)}
        c.font="12px sans-serif"
        if(ids.length===0){c.fillStyle="#8d9fb9";c.fillText("No current resource dependencies",24,40);return}
        if(ids.length>100){c.fillStyle="#f2bf65";c.fillText("Graph exceeds 100 nodes; inspect exported JSON",24,40);return}
        let points={};let rad=Math.max(20,Math.min(width,height)/2-65)
        for(let k=0;k<ids.length;k++){let a=2*Math.PI*k/ids.length;points[ids[k]]={x:width/2+Math.cos(a)*rad,y:height/2+Math.sin(a)*rad}}
        for(let r of resources){if(r.owner===null)continue;for(let w of r.waiting){
            let a=points[w],b=points[r.owner];if(!a||!b)continue
            let angle=Math.atan2(b.y-a.y,b.x-a.x),ex=b.x-27*Math.cos(angle),ey=b.y-27*Math.sin(angle)
            c.strokeStyle="#f2bf65";c.beginPath();c.moveTo(a.x,a.y);c.lineTo(ex,ey);c.stroke()
            c.beginPath();c.moveTo(ex,ey);c.lineTo(ex-12*Math.cos(angle-0.4),ey-12*Math.sin(angle-0.4))
            c.moveTo(ex,ey);c.lineTo(ex-12*Math.cos(angle+0.4),ey-12*Math.sin(angle+0.4));c.stroke()
            c.fillStyle="#f2bf65";c.fillText(r.id,(a.x+b.x)/2,(a.y+b.y)/2)
        }}
        let cycleIds=[];for(let d of (report.deadlocks || []))cycleIds=cycleIds.concat(d.cycle)
        for(let id of ids){let p=points[id];c.beginPath();c.arc(p.x,p.y,25,0,2*Math.PI);c.fillStyle=cycleIds.indexOf(id)>=0?"#a53d58":"#24665f";c.fill();c.fillStyle="#ffffff";c.fillText("J"+id,p.x-13,p.y+4)}
    }
}
