import { Renderer } from './renderer.js';

let Module = null;
let automataData = null;
let currentPhase = 'nfa';
let currentStep = 0;
let playing = false;
let playInterval = null;

async function init() {
    try {
        if (typeof AutomataModule !== 'undefined') {
            Module = await AutomataModule();
            document.getElementById('convert-btn').addEventListener('click', convert);
            setupControls();
        } else {
            document.getElementById('error-msg').innerText = "WASM module not loaded. Wait or run build script first.";
        }
    } catch (e) {
        document.getElementById('error-msg').innerText = "Error initializing WASM: " + e.message;
    }
}

function convert() {
    if (!Module) return;
    const regex = document.getElementById('regex-input').value;
    document.getElementById('error-msg').innerText = '';
    
    try {
        const json = Module.processRegex(regex);
        const result = JSON.parse(json);
        
        if (result.error) {
            document.getElementById('error-msg').innerText = result.error;
            return;
        }
        
        automataData = result;
        initPlayer('nfa');
    } catch (e) {
        document.getElementById('error-msg').innerText = "Error: " + e.message;
    }
}

function initPlayer(phase) {
    if (!automataData) return;
    
    currentPhase = phase;
    currentStep = 0;
    
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.target === phase);
    });
    
    renderStep(phase, 0);
}

function renderStep(phase, stepIndex) {
    if (!automataData) return;
    
    const steps = phase === 'nfa' ? automataData.nfaSteps : automataData.dfaSteps;
    const automaton = phase === 'nfa' ? automataData.nfa : automataData.dfa;
    
    if (!steps || steps.length === 0) {
        document.getElementById('step-desc').innerText = "No steps to display.";
        const states = phase === 'nfa' ? automataData.nfa.states : automataData.dfa.states;
        Renderer.draw(states, [], [], automaton.startState, true);
        return;
    }
    
    const step = steps[stepIndex];
    document.getElementById('step-desc').innerText = step.description;
    document.getElementById('step-counter').innerText = `${stepIndex + 1} / ${steps.length}`;
    
    Renderer.draw(step.snapshot, step.highlightStates || [], step.highlightEdges || [], automaton.startState, true);
}

function setupControls() {
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            if (playing) pause();
            initPlayer(e.target.dataset.target);
        });
    });
    
    document.getElementById('prev-btn').addEventListener('click', () => {
        if (!automataData) return;
        if (currentStep > 0) {
            currentStep--;
            renderStep(currentPhase, currentStep);
        }
    });
    
    document.getElementById('next-btn').addEventListener('click', () => {
        if (!automataData) return;
        const steps = currentPhase === 'nfa' ? automataData.nfaSteps : automataData.dfaSteps;
        if (currentStep < steps.length - 1) {
            currentStep++;
            renderStep(currentPhase, currentStep);
        }
    });
    
    document.getElementById('play-btn').addEventListener('click', () => {
        if (playing) {
            pause();
        } else {
            play();
        }
    });
    
    document.getElementById('speed-slider').addEventListener('input', () => {
        if (playing) {
            pause();
            play();
        }
    });
}

function play() {
    if (!automataData) return;
    const steps = currentPhase === 'nfa' ? automataData.nfaSteps : automataData.dfaSteps;
    if (currentStep >= steps.length - 1) {
        currentStep = 0;
    }
    
    playing = true;
    document.getElementById('play-btn').innerText = 'Pause';
    
    const speed = parseInt(document.getElementById('speed-slider').value);
    
    playInterval = setInterval(() => {
        if (currentStep < steps.length - 1) {
            currentStep++;
            renderStep(currentPhase, currentStep);
        } else {
            pause();
        }
    }, speed);
}

function pause() {
    playing = false;
    document.getElementById('play-btn').innerText = 'Play';
    if (playInterval) {
        clearInterval(playInterval);
        playInterval = null;
    }
}

window.addEventListener('DOMContentLoaded', init);
