export const Renderer = {
    draw(snapshot, highlightStates, highlightEdges, startId, acceptIdMode) {
        const canvas = document.getElementById('graph-canvas');
        const ctx = canvas.getContext('2d');
        const isDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
        
        const colors = {
            text: isDark ? '#f0f0f0' : '#111111',
            edge: isDark ? '#777777' : '#999999',
            edgeHighlight: isDark ? '#cc99ff' : '#9933cc',
            bg: isDark ? '#1a1a1a' : '#ffffff',
            stateNormal: isDark ? '#444444' : '#e0e0e0',
            stateStart: isDark ? '#336699' : '#99ccff',
            stateAccept: isDark ? '#2d862d' : '#99ff99',
            stateHighlight: isDark ? '#b38600' : '#ffcc00'
        };

        const ObjectValues = Object.values;
        const ObjectEntries = Object.entries;

        // Layout algorithm (BFS rank)
        const levels = {};
        const columns = [];
        
        let maxCol = 0;
        let start = snapshot.find(s => s.id === startId);
        
        if (!start && snapshot.length > 0) {
            start = snapshot[0];
        }

        if (start) {
            const q = [start.id];
            levels[start.id] = 0;
            
            while(q.length > 0) {
                const curr = q.shift();
                const col = levels[curr];
                maxCol = Math.max(maxCol, col);
                
                if (!columns[col]) columns[col] = [];
                if (!columns[col].includes(curr)) {
                    columns[col].push(curr);
                }
                
                const state = snapshot.find(s => s.id === curr);
                if (state) {
                    for (const [sym, dests] of ObjectEntries(state.transitions)) {
                        const destArray = Array.isArray(dests) ? dests : [dests];
                        for (const dest of destArray) {
                            if (levels[dest] === undefined) {
                                levels[dest] = col + 1;
                                q.push(dest);
                            }
                        }
                    }
                }
            }
        }
        
        for (const state of snapshot) {
            if (levels[state.id] === undefined) {
                levels[state.id] = 0;
                if (!columns[0]) columns[0] = [];
                columns[0].push(state.id);
            }
        }

        const maxStatesInAnyCol = columns.reduce((max, col) => Math.max(max, col ? col.length : 0), 0);
        
        const canvasWidth = (maxCol + 1) * 130 + 80;
        const canvasHeight = maxStatesInAnyCol * 90 + 80;
        
        canvas.width = Math.max(canvasWidth, 600);
        canvas.height = Math.max(canvasHeight, 300);

        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.fillStyle = colors.bg;
        ctx.fillRect(0, 0, canvas.width, canvas.height);

        const positions = {};
        for (let col = 0; col <= maxCol; col++) {
            if (!columns[col]) continue;
            const x = col * 130 + 60;
            const count = columns[col].length;
            const yStart = (canvas.height - (count - 1) * 90) / 2;
            
            for (let i = 0; i < count; i++) {
                const id = columns[col][i];
                positions[id] = { x, y: yStart + i * 90 };
            }
        }

        const radius = 22;

        ctx.font = '11px monospace';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';

        for (const state of snapshot) {
            const fromPos = positions[state.id];
            if (!fromPos) continue;

            for (const [sym, dests] of ObjectEntries(state.transitions)) {
                const destArray = Array.isArray(dests) ? dests : [dests];
                for (const dest of destArray) {
                    const toPos = positions[dest];
                    if (!toPos) continue;

                    const isHighlight = highlightEdges && highlightEdges.some(e => 
                        e[0] === state.id && e[1] === dest && e[2] === sym);
                    
                    ctx.strokeStyle = isHighlight ? colors.edgeHighlight : colors.edge;
                    ctx.lineWidth = isHighlight ? 2 : 1;
                    ctx.fillStyle = ctx.strokeStyle;

                    if (state.id === dest) {
                        ctx.beginPath();
                        ctx.moveTo(fromPos.x - 10, fromPos.y - radius);
                        ctx.bezierCurveTo(
                            fromPos.x - 30, fromPos.y - radius - 50,
                            fromPos.x + 30, fromPos.y - radius - 50,
                            fromPos.x + 10, fromPos.y - radius
                        );
                        ctx.stroke();
                        
                        ctx.beginPath();
                        ctx.moveTo(fromPos.x + 10, fromPos.y - radius);
                        ctx.lineTo(fromPos.x + 15, fromPos.y - radius - 10);
                        ctx.lineTo(fromPos.x + 5, fromPos.y - radius - 10);
                        ctx.fill();

                        ctx.fillStyle = colors.text;
                        ctx.fillText(sym, fromPos.x, fromPos.y - radius - 35);
                    } else {
                        let hasReverse = false;
                        const destState = snapshot.find(s => s.id === dest);
                        if (destState) {
                            for (const revDests of ObjectValues(destState.transitions)) {
                                const revArray = Array.isArray(revDests) ? revDests : [revDests];
                                if (revArray.includes(state.id)) {
                                    hasReverse = true;
                                    break;
                                }
                            }
                        }

                        const dx = toPos.x - fromPos.x;
                        const dy = toPos.y - fromPos.y;
                        const dist = Math.sqrt(dx*dx + dy*dy);
                        const nx = dx / dist;
                        const ny = dy / dist;
                        
                        let startX = fromPos.x + nx * radius;
                        let startY = fromPos.y + ny * radius;
                        let endX = toPos.x - nx * radius;
                        let endY = toPos.y - ny * radius;

                        let midX = (startX + endX) / 2;
                        let midY = (startY + endY) / 2;

                        if (hasReverse) {
                            const cx = midX - ny * 30;
                            const cy = midY + nx * 30;
                            
                            ctx.beginPath();
                            ctx.moveTo(startX, startY);
                            ctx.quadraticCurveTo(cx, cy, endX, endY);
                            ctx.stroke();

                            const t = 0.9;
                            const dirX = 2 * (1 - t) * (cx - startX) + 2 * t * (endX - cx);
                            const dirY = 2 * (1 - t) * (cy - startY) + 2 * t * (endY - cy);
                            const dirDist = Math.sqrt(dirX*dirX + dirY*dirY);
                            const dirNx = dirX / dirDist;
                            const dirNy = dirY / dirDist;

                            ctx.beginPath();
                            ctx.moveTo(endX, endY);
                            ctx.lineTo(endX - dirNx * 10 - dirNy * 5, endY - dirNy * 10 + dirNx * 5);
                            ctx.lineTo(endX - dirNx * 10 + dirNy * 5, endY - dirNy * 10 - dirNx * 5);
                            ctx.fill();

                            ctx.fillStyle = colors.text;
                            ctx.fillText(sym, cx - ny * 10, cy + nx * 10);

                        } else {
                            ctx.beginPath();
                            ctx.moveTo(startX, startY);
                            ctx.lineTo(endX, endY);
                            ctx.stroke();

                            ctx.beginPath();
                            ctx.moveTo(endX, endY);
                            ctx.lineTo(endX - nx * 10 - ny * 5, endY - ny * 10 + nx * 5);
                            ctx.lineTo(endX - nx * 10 + ny * 5, endY - ny * 10 - nx * 5);
                            ctx.fill();

                            ctx.fillStyle = colors.text;
                            ctx.fillText(sym, midX - ny * 15, midY + nx * 15);
                        }
                    }
                }
            }
        }

        if (start && positions[start.id]) {
            const p = positions[start.id];
            ctx.fillStyle = colors.edge;
            ctx.beginPath();
            ctx.moveTo(p.x - radius, p.y);
            ctx.lineTo(p.x - radius - 20, p.y - 10);
            ctx.lineTo(p.x - radius - 20, p.y + 10);
            ctx.fill();
        }

        for (const state of snapshot) {
            const p = positions[state.id];
            if (!p) continue;

            const isStart = (state.id === startId);
            const isHighlight = highlightStates && highlightStates.includes(state.id);

            ctx.fillStyle = isHighlight ? colors.stateHighlight : 
                            (isStart ? colors.stateStart : 
                            (state.accept ? colors.stateAccept : colors.stateNormal));
            
            ctx.strokeStyle = colors.text;
            ctx.lineWidth = 1;

            ctx.beginPath();
            ctx.arc(p.x, p.y, radius, 0, Math.PI * 2);
            ctx.fill();
            ctx.stroke();

            if (state.accept) {
                ctx.beginPath();
                ctx.arc(p.x, p.y, radius - 4, 0, Math.PI * 2);
                ctx.stroke();
            }

            ctx.fillStyle = colors.text;
            ctx.font = '12px monospace';
            ctx.fillText(state.id === undefined ? "" : "q" + state.id, p.x, p.y);
        }
    }
};
