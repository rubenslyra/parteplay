import { TempoSuggestionPanel } from "@/components/TempoSuggestionPanel";
import { createFileRoute } from "@tanstack/react-router";
import { useCallback, useEffect, useRef, useState, type ChangeEvent, type DragEvent, type MouseEvent } from "react";
import { ArrowDownToLine, FileAudio2, Headphones, Pause, Play, RotateCcw, SkipBack, Upload, Volume2, VolumeX, Waves } from "lucide-react";
import { Button } from "@/components/ui/button";
import partePlayLogo from "@/assets/PartePlay-Icon.png.asset.json";

export const Route = createFileRoute("/")({
  head: () => ({
    meta: [
      { title: "PartePlay — Interface de áudio e partitura | Rubinho Lyra" },
      { name: "description", content: "Prévia interativa do PartePlay: visualize áudio, explore instrumentos transpositores e ajuste a afinação de referência. Produção Rubinho Lyra." },
      { property: "og:title", content: "PartePlay — Áudio e partitura no mesmo compasso" },
      { property: "og:description", content: "Conheça a interface do PartePlay, criada por Rubinho Lyra para estudo musical e arranjo." },
      { property: "og:type", content: "website" },
      { name: "twitter:card", content: "summary_large_image" },
    ],
  }),
  component: Index,
});

type Instrument = { name: string; key: string; semitones: number; symbol: string };
const instruments: Instrument[] = [
  { name: "Piano", key: "Dó", semitones: 0, symbol: "C" },
  { name: "Trombone", key: "Dó", semitones: 0, symbol: "C" },
  { name: "Trompete", key: "Si♭", semitones: -2, symbol: "B♭" },
  { name: "Sax Tenor", key: "Si♭", semitones: -2, symbol: "B♭" },
  { name: "Sax Alto", key: "Mi♭", semitones: 3, symbol: "E♭" },
  { name: "Trompa", key: "Fá", semitones: 5, symbol: "F" },
];
const demoPeaks = Array.from({ length: 160 }, (_, i) => {
  const envelope = Math.min(1, i / 18, (160 - i) / 25);
  return (0.1 + Math.abs(Math.sin(i * 0.47) * Math.cos(i * 0.113)) * 0.7 + Math.abs(Math.sin(i * 1.39)) * 0.17) * Math.max(0.1, envelope);
});

function timeLabel(value: number) {
  if (!Number.isFinite(value)) return "00:00";
  const minutes = Math.floor(value / 60);
  const seconds = Math.floor(value % 60);
  return `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
}

function Waveform({ peaks, progress, onSeek, loaded }: { peaks: number[]; progress: number; onSeek: (fraction: number) => void; loaded: boolean }) {
  const seek = (event: MouseEvent<HTMLDivElement>) => {
    if (!loaded) return;
    const rect = event.currentTarget.getBoundingClientRect();
    onSeek(Math.max(0, Math.min(1, (event.clientX - rect.left) / rect.width)));
  };
  return (
    <div
      className={`wave-grid relative flex h-44 items-center overflow-hidden rounded-md border border-line px-2 sm:h-52 ${loaded ? "cursor-crosshair" : ""}`}
      onClick={seek}
      role={loaded ? "slider" : "img"}
      aria-label={loaded ? "Posição de reprodução" : "Forma de onda ilustrativa; carregue um áudio para começar"}
      aria-valuemin={loaded ? 0 : undefined}
      aria-valuemax={loaded ? 100 : undefined}
      aria-valuenow={loaded ? Math.round(progress * 100) : undefined}
      tabIndex={loaded ? 0 : undefined}
      onKeyDown={(event) => {
        if (!loaded) return;
        if (event.key === "ArrowRight") onSeek(Math.min(1, progress + 0.02));
        if (event.key === "ArrowLeft") onSeek(Math.max(0, progress - 0.02));
      }}
    >
      <div className="absolute inset-x-0 top-1/2 h-px bg-primary/20" />
      <div className="relative z-10 flex h-full w-full items-center gap-px" aria-hidden="true">
        {peaks.map((peak, i) => (
          <div key={i} className={`min-w-0 flex-1 rounded-full transition-colors duration-150 ${loaded && i / peaks.length <= progress ? "bg-primary" : "bg-primary/45"}`} style={{ height: `${Math.max(3, peak * 78)}%` }} />
        ))}
      </div>
      {loaded && <div className="absolute inset-y-0 z-20 w-px bg-ink/70" style={{ left: `${progress * 100}%` }} />}
      {!loaded && <div className="pointer-events-none absolute inset-0 z-20 flex items-center justify-center bg-card/25"><span className="rounded-md border border-line bg-card/85 px-4 py-2 text-xs font-medium text-mid shadow-sm backdrop-blur-sm">Carregue um áudio para visualizar a forma de onda</span></div>}
    </div>
  );
}

function SplashScreen() {
  const [leaving, setLeaving] = useState(false);
  const [gone, setGone] = useState(false);
  useEffect(() => {
    const fade = window.setTimeout(() => setLeaving(true), 1900);
    const remove = window.setTimeout(() => setGone(true), 2600);
    return () => { window.clearTimeout(fade); window.clearTimeout(remove); };
  }, []);
  if (gone) return null;
  return (
    <div className={`splash-screen fixed inset-0 z-50 flex flex-col items-center justify-center ${leaving ? "splash-leaving" : ""}`} aria-hidden="true">
      <img className="splash-logo h-auto w-[min(78vw,340px)] object-contain" src={partePlayLogo.url} alt="" />
      <div className="mt-8 h-1 w-44 overflow-hidden rounded-full bg-line"><div className="splash-bar h-full rounded-full bg-primary" /></div>
      <div className="splash-subtitle mt-6 text-[10px] font-semibold uppercase text-mid">Produção Rubinho Lyra · Software Eng</div>
    </div>
  );
}

function Index() {
  const [instrument, setInstrument] = useState<Instrument>(instruments[2] ?? { name: "Trompete", key: "Si♭", semitones: -2, symbol: "B♭" });
  const [tuning, setTuning] = useState(440);
  const [fileName, setFileName] = useState("");
  const [fileType, setFileType] = useState("");
  const [fileSize, setFileSize] = useState("");
  const [duration, setDuration] = useState(0);
  const [currentTime, setCurrentTime] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const [loop, setLoop] = useState(false);
  const [muted, setMuted] = useState(false);
  const [peaks, setPeaks] = useState<number[]>(demoPeaks);
  const [error, setError] = useState("");
  const [dragOver, setDragOver] = useState(false);
  const [currentFile, setCurrentFile] = useState<File | null>(null);
  const audioRef = useRef<HTMLAudioElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);
  const urlRef = useRef<string | null>(null);
  const loopRef = useRef(loop);
  loopRef.current = loop;

  useEffect(() => () => { if (urlRef.current) URL.revokeObjectURL(urlRef.current); }, []);

  const loadFile = useCallback(async (file?: File) => {
    if (!file) return;
    if (!(/\.(wav|mp3|flac)$/i.test(file.name) || ["audio/wav", "audio/mpeg", "audio/flac", "audio/x-flac"].includes(file.type))) {
      setError("Selecione um arquivo WAV, FLAC ou MP3.");
      return;
    }
    setError("");
    setCurrentFile(file);
    setIsPlaying(false);
    setCurrentTime(0);
    setDuration(0);
    setPeaks(demoPeaks);
    if (audioRef.current) audioRef.current.pause();
    if (urlRef.current) URL.revokeObjectURL(urlRef.current);
    const url = URL.createObjectURL(file);
    urlRef.current = url;
    setFileName(file.name);
    setFileType(file.name.split(".").pop()?.toUpperCase() || "ÁUDIO");
    setFileSize(`${(file.size / 1024 / 1024).toFixed(1)} MB`);
    if (audioRef.current) audioRef.current.src = url;
    try {
      const context = new AudioContext();
      try {
        const buffer = await file.arrayBuffer();
        const decoded = await context.decodeAudioData(buffer);
        const channel = decoded.getChannelData(0);
        const count = 160;
        const block = Math.max(1, Math.floor(channel.length / count));
        const next = Array.from({ length: count }, (_, index) => {
          let max = 0;
          const start = index * block;
          for (let j = start; j < Math.min(start + block, channel.length); j += Math.max(1, Math.floor(block / 250))) {
            max = Math.max(max, Math.abs(channel[j] ?? 0));
          }
          return Math.max(0.025, max);
        });
        setPeaks(next);
      } finally {
        await context.close();
      }
    } catch {
      // The audio element may still play a format the waveform decoder cannot read.
      setPeaks(demoPeaks);
    }
  }, []);

  const onFileChange = (event: ChangeEvent<HTMLInputElement>) => {
    void loadFile(event.target.files?.[0]);
    event.target.value = "";
  };
  const onDrop = (event: DragEvent<HTMLDivElement>) => {
    event.preventDefault();
    setDragOver(false);
    void loadFile(event.dataTransfer.files[0]);
  };
  const seek = (fraction: number) => {
    const audio = audioRef.current;
    if (!audio || !duration) return;
    audio.currentTime = fraction * duration;
    setCurrentTime(audio.currentTime);
  };
  const togglePlay = async () => {
    const audio = audioRef.current;
    if (!audio || !fileName) { inputRef.current?.click(); return; }
    if (audio.paused) {
      try { await audio.play(); setError(""); }
      catch { setError("Não foi possível reproduzir este arquivo no navegador."); }
    } else audio.pause();
  };
  const stop = () => {
    const audio = audioRef.current;
    if (!audio) return;
    audio.pause();
    audio.currentTime = 0;
    setCurrentTime(0);
  };
  const progress = duration ? currentTime / duration : 0;

  return (
    <main className="studio-backdrop min-h-screen px-4 py-6 sm:px-6 sm:py-10 lg:py-14">
      <SplashScreen />
      <div className="mx-auto w-full max-w-[1160px]">
        <div className="mb-6 flex flex-wrap items-end justify-between gap-3 px-1">
          <div>
            <div className="mb-2 flex items-center gap-2 text-[11px] font-bold uppercase text-mid"><span className="h-1.5 w-1.5 rounded-full bg-teal" /> ESTÚDIO / PARTE<span className="text-primary">PLAY</span></div>
            <h1 className="text-2xl font-extrabold text-ink sm:text-3xl">Áudio e partitura, em sintonia.</h1>
          </div>
          <div className="text-right text-xs text-mid">PARTEPLAY <span className="mx-1 text-primary">/</span> VISUAL PREVIEW 01</div>
        </div>

        <div className="plugin-shell overflow-hidden rounded-lg border border-line sm:rounded-[22px]">
          <div className="plugin-header flex flex-wrap items-center justify-between gap-3 border-b border-line px-5 py-4 sm:px-7">
            <div className="flex min-w-0 items-center gap-3">
              <img className="h-11 w-11 shrink-0 object-contain drop-shadow-md" src={partePlayLogo.url} alt="Logo PartePlay" />
              <div className="min-w-0">
                <div className="text-[15px] font-extrabold text-ink">PartePlay</div>
                <div className="text-[10px] font-semibold uppercase text-mid">REFERENCE AUDIO WORKSPACE</div>
              </div>
            </div>
            <div className="flex items-center gap-2">
              <span className="rounded-md border border-teal/20 bg-teal/10 px-2.5 py-1 text-[10px] font-bold text-teal">CONCEITO VST3</span>
              <span className="rounded-md border border-line bg-surface px-2.5 py-1 text-[10px] font-bold text-mid">PRÉVIA WEB</span>
            </div>
          </div>

          <div className="p-4 sm:p-6 lg:p-7">
            <div className="grid gap-4 lg:grid-cols-[1.7fr_1fr]">
              <div className="space-y-4">
                <section className={`glass-panel rounded-lg border p-4 sm:p-5 ${dragOver ? "border-primary" : "border-line"}`} onDragOver={(event) => { event.preventDefault(); setDragOver(true); }} onDragLeave={() => setDragOver(false)} onDrop={onDrop} aria-label="Arquivo de áudio">
                  <div className="mb-4 flex flex-wrap items-center justify-between gap-3">
                    <div className="flex min-w-0 items-center gap-3">
                      <span className="grid h-9 w-9 shrink-0 place-items-center rounded-md bg-primary/10 text-primary"><FileAudio2 size={18} /></span>
                      <div className="min-w-0"><p className="max-w-[22ch] truncate text-sm font-bold text-ink sm:max-w-[34ch]" title={fileName}>{fileName || "Nenhum áudio carregado"}</p><p className="text-[11px] text-mid">{fileName ? `${fileType} · ${fileSize}` : "WAV, FLAC ou MP3"}</p></div>
                    </div>
                    <Button variant="studio" size="sm" onClick={() => inputRef.current?.click()}><Upload size={15} />{fileName ? "Trocar áudio" : "Carregar áudio"}</Button>
                    <input ref={inputRef} type="file" accept=".wav,.flac,.mp3,audio/*" className="hidden" onChange={onFileChange} aria-label="Selecionar arquivo de áudio" />
                  </div>
                  <Waveform peaks={peaks} progress={progress} onSeek={seek} loaded={!!fileName && duration > 0} />
                  <div className="mt-3 flex items-center justify-between text-[11px] font-medium text-mid"><span className="font-mono">{timeLabel(currentTime)}</span><span className="text-center text-primary">{fileName ? "Clique na forma de onda para navegar" : "Arraste seu arquivo até aqui"}</span><span className="font-mono">{timeLabel(duration)}</span></div>
                  {error && <p role="alert" className="mt-3 text-xs font-semibold text-destructive">{error}</p>}
                </section>

                <section className="glass-panel rounded-lg border border-line p-4 sm:p-5" aria-label="Transporte">
                  <div className="mb-4 flex items-center justify-between"><p className="text-[10px] font-bold uppercase text-mid">TRANSPORTE</p><span className="flex items-center gap-1.5 text-[10px] font-bold uppercase text-mid"><span className="h-1.5 w-1.5 rounded-full bg-signal" /> MODO LOCAL</span></div>
                  <div className="flex flex-wrap items-center gap-2 sm:gap-3">
                    <Button variant="studio-primary" size="icon" className="h-11 w-11 rounded-full" onClick={() => void togglePlay()} title={isPlaying ? "Pausar" : "Reproduzir"} aria-label={isPlaying ? "Pausar" : "Reproduzir"}>{isPlaying ? <Pause size={18} fill="currentColor" /> : <Play size={18} fill="currentColor" />}</Button>
                    <Button variant="studio" size="icon" className="rounded-full" onClick={stop} title="Parar" aria-label="Parar"><SkipBack size={16} fill="currentColor" /></Button>
                    <Button variant={loop ? "studio-active" : "studio"} size="icon" className="rounded-full" onClick={() => setLoop(!loop)} title={loop ? "Desativar repetição" : "Ativar repetição"} aria-label="Repetição" aria-pressed={loop}><RotateCcw size={16} /></Button>
                    <div className="mx-1 hidden h-8 w-px bg-line sm:block" />
                    <Button variant={muted ? "studio-active" : "studio"} size="icon" className="rounded-full" onClick={() => setMuted(!muted)} title={muted ? "Ativar som" : "Silenciar"} aria-label={muted ? "Ativar som" : "Silenciar"}>{muted ? <VolumeX size={17} /> : <Volume2 size={17} />}</Button>
                    <div className="ml-auto min-w-[86px] rounded-md border border-line bg-surface px-3 py-2 text-right font-mono text-[13px] text-ink">{timeLabel(currentTime)} <span className="text-mid">/ {timeLabel(duration)}</span></div>
                  </div>
                  <div className="mt-5 flex flex-wrap items-center justify-between gap-2 border-t border-line pt-4 text-[11px] text-mid"><span className="flex items-center gap-2"><Headphones size={14} className="text-primary" /> Reprodução local no navegador</span><span>SYNC HOST <span className="font-bold text-signal">INDISPONÍVEL NA PRÉVIA</span></span></div>
                </section>
              </div>

              <div className="space-y-4">
                <section className="glass-panel rounded-lg border border-line p-4 sm:p-5" aria-label="Instrumento">
                  <p className="mb-4 text-[10px] font-bold uppercase text-mid">01 / INSTRUMENTO</p>
                  <div className="flex items-center gap-4">
                    <div className="knob-ring relative grid h-16 w-16 shrink-0 place-items-center rounded-full"><div className="absolute inset-[7px] rounded-full border border-line bg-card" /><span className="relative font-mono text-lg font-bold text-primary">{instrument.symbol}</span></div>
                    <div><p className="text-lg font-extrabold text-ink">{instrument.name}</p><p className="text-xs text-mid">Afinação em {instrument.key} <span className="mx-1">·</span> {instrument.semitones > 0 ? "+" : ""}{instrument.semitones} semitons</p></div>
                  </div>
                  <div className="mt-5 grid grid-cols-2 gap-2">
                    {instruments.map((item) => <Button key={item.name} variant={item.name === instrument.name ? "studio-active" : "studio"} size="sm" className="h-auto min-h-10 justify-between px-3 py-2 text-left text-[11px] sm:text-xs" onClick={() => setInstrument(item)} aria-pressed={item.name === instrument.name}><span className="truncate">{item.name}</span><span className="ml-1 font-mono text-[10px] opacity-70">{item.symbol}</span></Button>)}
                  </div>
                </section>

                <section className="glass-panel rounded-lg border border-line p-4 sm:p-5" aria-label="Afinação e transposição">
                  <p className="mb-4 text-[10px] font-bold uppercase text-mid">02 / AFINAÇÃO & TRANSPOSIÇÃO</p>
                  <div className="flex items-end justify-between gap-4"><div className="knob-ring relative grid h-12 w-12 shrink-0 place-items-center rounded-full"><div className="absolute inset-[6px] rounded-full border border-line bg-card" /><span className="relative text-lg text-primary">♪</span></div><div className="text-right"><p className="text-[11px] font-medium text-mid">Referência · A4</p><p className="font-mono text-2xl font-medium text-ink">{tuning}<span className="ml-1 text-xs text-mid">Hz</span></p></div></div>
                  <div className="mt-5 flex items-center gap-3 rounded-md border border-line bg-surface px-3 py-3"><span className="font-mono text-[11px] text-mid">432</span><input type="range" min="432" max="445" step="1" value={tuning} onChange={(event) => setTuning(Number(event.target.value))} className="track-range w-full cursor-pointer" aria-label="Afinação de referência em hertz" /><span className="font-mono text-[11px] text-mid">445</span></div>
                  <div className="mt-3 flex flex-wrap gap-2"><Button variant="studio" size="sm" className="h-7 px-2 text-[10px]" onClick={() => setTuning(440)}><RotateCcw size={12} /> Restaurar 440 Hz</Button></div>
                  <div className="mt-4 grid grid-cols-2 gap-2"><div className="rounded-md border border-line bg-surface p-3"><p className="text-[10px] text-mid">TRANSPOSIÇÃO</p><p className="mt-1 font-mono text-lg text-ink">{instrument.semitones > 0 ? "+" : ""}{instrument.semitones} <span className="text-xs text-mid">st</span></p></div><div className="rounded-md border border-line bg-surface p-3"><p className="text-[10px] text-mid">REFERÊNCIA</p><p className="mt-1 font-mono text-lg text-ink">A4 <span className="text-xs text-mid">/ {tuning} Hz</span></p></div></div>
                  <p className="mt-3 text-[11px] leading-relaxed text-mid">A transposição é exibida como referência visual; esta prévia não altera a altura do áudio.</p>
                </section>
                <TempoSuggestionPanel file={currentFile} />
              </div>
            </div>
            <footer className="mt-6 flex flex-wrap items-center justify-between gap-3 border-t border-line pt-5 text-[11px] text-mid"><span>PartePlay <span className="mx-1 text-primary">·</span> Feito para músicos e arranjadores</span><span>PRODUÇÃO <span className="font-bold text-ink">RUBINHO LYRA</span> <span className="mx-1 text-primary">/</span> SOFTWARE ENG</span></footer>
          </div>
        </div>
        <div className="mt-5 flex flex-wrap items-center justify-between gap-3 px-1 text-[11px] text-mid"><span>Template visual interativo · A integração VST3 e a sincronia com MuseScore não operam nesta prévia web.</span><span className="flex items-center gap-1.5"><ArrowDownToLine size={13} /> PARTE<span className="font-bold text-primary">PLAY</span> LAB</span></div>
      </div>
      <audio ref={audioRef} preload="metadata" muted={muted} onLoadedMetadata={(event) => setDuration(event.currentTarget.duration)} onTimeUpdate={(event) => setCurrentTime(event.currentTarget.currentTime)} onPlay={() => setIsPlaying(true)} onPause={() => setIsPlaying(false)} onEnded={(event) => { if (loopRef.current) { event.currentTarget.currentTime = 0; void event.currentTarget.play(); } else setIsPlaying(false); }} onError={() => { if (fileName) setError("Este formato não pôde ser reproduzido neste navegador."); }} />
    </main>
  );
}
